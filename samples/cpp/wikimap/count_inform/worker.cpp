#include "releases_notification.h"

#include <yandex/maps/grinder/worker/api.h>

#include <yandex/maps/cmdline.h>
#include <yandex/maps/common/exception.h>
#include <yandex/maps/log8/log.h>
#include <yandex/maps/pgpool3/pgpool3.h>
#include <yandex/maps/wiki/common/geom.h>
#include <yandex/maps/wiki/common/default_config.h>
#include <yandex/maps/wiki/common/extended_xml_doc.h>
#include <yandex/maps/wiki/common/pgpool3_helpers.h>
#include <yandex/maps/wiki/common/task_logger.h>
#include <yandex/maps/wiki/revision/common.h>
#include <yandex/maps/wiki/common/long_tasks.h>

#include <memory>
#include <string>

#include <boost153/lexical_cast.hpp>

namespace common = maps::wiki::common;
namespace worker = maps::grinder::worker;
namespace revision = maps::wiki::revision;
namespace rn = maps::wiki::releases_notification;
namespace tasks = maps::wiki::tasks;

namespace {

const std::string TASK_TYPE = "releases_notification";

const size_t WORKER_CONCURRENCY = 1;

} // anonymous namespace

int main(int argc, char** argv)
{
    try {
        maps::cmdline::Parser parser;

        auto syslogTag = parser.string("syslog-tag").help(
            "redirect log output to syslog with given tag");
        auto workerConfig = parser.file("config").help(
            "path to services configuration");
        auto grinderConfig = parser.file("grinder-config").help(
            "path to grinder configuration");

        parser.parse(argc, argv);

        if (syslogTag.defined()) {
            maps::log8::setBackend(maps::log8::toSyslog(syslogTag));
        }

        std::unique_ptr<common::ExtendedXmlDoc> configDocPtr;
        if (workerConfig.defined()) {
            configDocPtr.reset(new common::ExtendedXmlDoc(workerConfig));
        } else {
            configDocPtr = common::loadDefaultConfig();
        }

        worker::Options options;
        if (grinderConfig.defined()) {
            options.setConfigLocation(grinderConfig);
        }

        options.setConcurrencyLevel(WORKER_CONCURRENCY);
        options.on(TASK_TYPE, [&configDocPtr](const worker::Task& task)
        {
            INFO() << "Received releases notification task, id: " << task.id();

            common::PoolHolder corePoolHolder(
                *configDocPtr, "core", "core");
            common::PoolHolder longReadPoolHolder(
                *configDocPtr, "long-read", "long-read");
            auto& corePool = corePoolHolder.pool();
            auto& longReadPool = longReadPoolHolder.pool();

            rn::ReleasesNotificationConfig localConfig(*configDocPtr);
            auto dbTaskId = task.args()["task-id"].as<revision::DBID>();

            maps::wiki::tasks::TaskPgLogger logger(corePool, dbTaskId);
            bool frozen = tasks::isFrozen(*corePool.masterReadOnlyTransaction(), dbTaskId);
            logger.logInfo() << "Task " << (frozen ? "resumed" : "started")
                             << ". Grinder task id: " << task.id();

            rn::TaskParams params(*corePool.masterReadOnlyTransaction(), dbTaskId);

            if (not frozen) {
                logger.logInfo() << "Retrieve users for release";
                auto usersNumber = calculateAndSaveUsersForRelease(
                    corePool,
                    longReadPool,
                    localConfig,
                    params);
                logger.logInfo() << "Get " << usersNumber << " users to notify";
            }

            if (task.isCanceled()) {
                WARN() << "Task " << task.id() << ": cancelled before sending";
                logger.logInfo() << "Task cancelled";
                throw worker::TaskCanceledException();
            }
            logger.logInfo() << "Notification started";

            std::unique_ptr<rn::ISender> sender;
            if (params.mode() == rn::Mode::Dry) {
                sender.reset(new rn::DrySender());
            } else {
                rn::SenderConfiguration senderConf;
                senderConf.host = localConfig.stringParam("/sender/url");
                senderConf.userId = localConfig.stringParam("/sender/user-id");
                senderConf.accountSlug = localConfig.stringParam("/sender/account-slug");
                if (params.releaseType() == rn::ReleaseType::Vec) {
                    senderConf.campaignSlug = localConfig.stringParam("/sender/campaign-slug-vec");
                } else {
                    senderConf.campaignSlug = localConfig.stringParam("/sender/campaign-slug-sat");
                }
                sender.reset(new rn::Sender(senderConf, rn::RetryPolicy::defaultPolicy()));
            }

            notifyUsers(corePoolHolder.pool(), dbTaskId, *sender, logger);

            if (frozen) {
                tasks::unfreezeTask(*corePool.masterWriteableTransaction(), dbTaskId);
            }

            if (task.isCanceled()) {
                WARN() << "Task " << task.id() << ": cancelled after sending";
                logger.logWarn() << "Task cancelled";
                throw worker::TaskCanceledException();
            } else {
                INFO() << "Task " << task.id() << ": completed succesfully";
                logger.logInfo() << "Task completed succesfully";
            }
        });

        worker::run(std::move(options));

        return EXIT_SUCCESS;
    } catch (const maps::Exception& ex) {
        FATAL() << "Worker failed: " << ex;
        return EXIT_FAILURE;
    } catch (const std::exception& ex) {
        FATAL() << "Worker failed: " << ex.what();
        return EXIT_FAILURE;
    }
}
