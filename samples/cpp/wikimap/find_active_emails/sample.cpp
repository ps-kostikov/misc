#include "blackbox.h"

#include <yandex/maps/pgpool3/pgpool3.h>
#include <yandex/maps/pgpool3/adapter_helpers.h>

#include <yandex/maps/wiki/acl/aclgateway.h>
#include <yandex/maps/wiki/acl/acl.h>

#include <yandex/maps/wiki/revision/branch.h>
#include <yandex/maps/wiki/revision/branch_manager.h>
#include <yandex/maps/wiki/revision/commit.h>
#include <yandex/maps/wiki/revision/snapshot.h>
#include <yandex/maps/wiki/revision/revisionsgateway.h>
#include <yandex/maps/wiki/revision/snapshot_id.h>
#include <yandex/maps/wiki/revision/exception.h>

#include <yandex/maps/wiki/social/gateway.h>
#include <yandex/maps/wiki/social/feed.h>
#include <yandex/maps/wiki/social/moderation.h>

#include <yandex/maps/wiki/common/date_time.h>
#include <yandex/maps/wiki/common/paged_result.h>

#include <iostream>
#include <string>

namespace mpgp = maps::pgpool3;
namespace mw = maps::wiki;
namespace mwr = maps::wiki::revision;
namespace mws = maps::wiki::social;
namespace mwa = maps::wiki::acl;
namespace mwrn = mw::releases_notification;

class PgpoolLogger: public mpgp::logging::Logger
{
public:
    PgpoolLogger()
    { }

    virtual void put(mpgp::logging::Message&& /*msg*/) override
    {
        // std::cout << msg.source << " " << msg.message << std::endl;
    }

    virtual mpgp::logging::Level level() const override
    {
        return mpgp::logging::Level::Debug;
    }

    virtual std::string name() const override
    {
        return "pgppool3";
    }
};


std::set<mws::TUid> findBannedUids(maps::pqxx::transaction_base& work)
{
    auto rows = work.exec("select br_uid from acl.ban_record where br_expires is NULL and br_action='ban'");
    std::set<mws::TUid> result;
    for (const auto& row: rows) {
        result.insert(row["br_uid"].as<mws::TUid>());
    }
    return result;
}


void evalSomething(maps::pgpool3::Pool& pool)
{
    auto txn = pool.slaveTransaction();

    mwa::ACLGateway agw(*txn);
    mws::Gateway sgw(*txn);
    mwrn::Blackbox blackbox(
        mwrn::BlackboxConfiguration(
            "blackbox-ipv6.yandex.net",
            mwrn::RetryPolicy::defaultPolicy()
        )
    );

    auto printEmailFromBlackbox = [&blackbox](mwa::UID uid) {
        auto socialInfo = blackbox.userInfo(uid);
        if (socialInfo) {
            std::cout << uid << "," << socialInfo->email << std::endl;
        }
    };

    auto maxCreated = mw::common::parseSqlDateTime("2016-09-01 23:59:59.0+03");

    auto users = agw.users();

    std::vector<mwa::UID> uids;
    for (const auto& user: users) {
        uids.push_back(user.uid());
    }
    auto profiles = sgw.getUserProfiles(std::set<mws::TId>(uids.begin(), uids.end()));
    std::map<mwa::UID, mws::Profile> uidToProfile;
    for (const auto& profile: profiles) {
        uidToProfile.emplace(std::make_pair(profile.uid(), profile));
    }

    size_t N = 0;
    for (const auto& user: users) {
        if (user.status() != mwa::User::Status::Active) {
            continue;
        }

        if (mw::common::parseSqlDateTime(user.created()) > maxCreated) {
            continue;
            // std::cout << user.created() << std::endl;            
        }

        if (uidToProfile.count(user.uid()) > 0) {
            const auto& profile = uidToProfile.at(user.uid());
            if (not profile.email().empty()) {
                std::cout << user.uid() << "," << profile.email() << std::endl;
            } else {
                printEmailFromBlackbox(user.uid());
            }
        } else {
            printEmailFromBlackbox(user.uid());
        }

        ++N;
        // if (N > 100) {
        //     break;
        // }
    }


    // std::cout << users.size() << std::endl;

    // int s = 0;
    // int ss = 0;
    // auto bannedUids = findBannedUids(*txn);
    // for (const auto& user: users) {
    //     if (bannedUids.count(user.uid()) == 0) {
    //         ++s;
    //     }
    //     if (user.status() == mwa::User::Status::Active) {
    //         ++ss;
    //     }
    //     if (bannedUids.count(user.uid()) > 0 and user.status() == mwa::User::Status::Active) {
    //         std::cout << "strange: " << user.uid() << " " << user.login() << std::endl;
    //     }
    // }
    // std::cout << s << std::endl;
    // std::cout << ss << std::endl;
}



int main(int argc, const char** argv)
{

    // std::cout << "hello" << std::endl;
    // std::cout << "argc = " << argc << std::endl;
    if (argc < 2) {
        std::cout << "expected cmd line: ./<exe> <connection string>" << std::endl;
        return 1;
    }

    std::string connStr(argv[1]);

    // std::cout << "connection string = '" << connStr << "'" << std::endl;
    mpgp::PoolConfigurationPtr poolConfiguration(mpgp::PoolConfiguration::create());
    std::string authParams;
    std::tie(poolConfiguration->master, authParams) = mpgp::helpers::parseConnString(connStr);
    mpgp::PoolConstantsPtr poolConstants(mpgp::PoolConstants::create(1, 1, 0, 0));
    poolConstants->getTimeoutMs = 5000;
    poolConstants->pingIntervalMs = 5000;

    PgpoolLogger logger;
    std::unique_ptr<mpgp::Pool> pool(new mpgp::Pool(
        logger,
        std::move(poolConfiguration),
        authParams,
        std::move(poolConstants)
    ));

    evalSomething(*pool);

    return 0;
}
