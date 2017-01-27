#include <yandex/maps/yacare.h>
#include <yandex/maps/pgpool3/pgpool3.h>
#include <yandex/maps/json/builder.h>
#include <yandex/maps/json/value.h>

#include <boost/optional.hpp>

#include <chrono>
#include <iostream>
#include <sstream>

namespace mpgp = maps::pgpool3;
namespace mj = maps::json;

std::unique_ptr<mpgp::Pool> g_poolPtr;

class PgpoolLogger: public mpgp::logging::Logger
{
public:
    PgpoolLogger()
    { }

    virtual void put(mpgp::logging::Message&& msg) override
    {
        std::cout << msg.source << " " << msg.message << std::endl;
    }

    virtual mpgp::logging::Level level() const override
    {
        return mpgp::logging::Level::Info;
    }

    virtual std::string name() const override
    {
        return "pgppool3";
    }
};


template <class T>
boost::optional<T>
getOptionalParam(const yacare::Request& r, const std::string& name)
{
    std::string strParam = r.input()[name];
    if (strParam.empty()) {
        return boost::none;
    }
    try {
        return boost::lexical_cast<T>(strParam);
    } catch (const boost::bad_lexical_cast&) {
        throw yacare::errors::BadRequest() << " Invalid parameter: " << name << " : " << strParam;
    }
}


// yacare::ThreadPool heavyPool(/* name = */ "heavy", /* threads  = */ 32, /* backlog = */ 16);
// YCR_RESPOND_TO("sample:/signals-renderer", YCR_IN_POOL(heavyPool))

YCR_RESPOND_TO("sample:/tiles")
{
    auto xOpt = getOptionalParam<uint64_t>(request, "x");
    auto yOpt = getOptionalParam<uint64_t>(request, "y");
    auto zOpt = getOptionalParam<uint64_t>(request, "z");
    if (not xOpt or not yOpt or not zOpt) {
        throw yacare::errors::BadRequest();
    }
    auto callbackOpt = getOptionalParam<std::string>(request, "callback");

    // auto connection = g_poolPtr->getMasterConnection();
    // auto transaction = mpgp::makeWriteableTransaction(std::move(connection));
    // std::stringstream query;
    // // query << "select mds_key from signals.signals_tiles where style = 'point'"
    // query << "select mds_key from signals_tiles where style = 'point'"
    //     << " and x = " << *xOpt
    //     << " and y = " << *yOpt
    //     << " and z = " << *zOpt
    //     << ";";
    // auto postgresQueryBegin = std::chrono::system_clock::now();
    // auto result = transaction->exec(query.str());
    // auto postgresQueryEnd = std::chrono::system_clock::now();
    // std::chrono::duration<double> postgresQueryDuration = postgresQueryEnd - postgresQueryBegin;

    // if (result.empty()) {
    //     std::cout << "postgres time " << postgresQueryDuration.count() << "sec" << std::endl;
    //     throw yacare::errors::NotFound() << "no db record";
    // }

// GET /signals-renderer?x=74533&y=45031&z=17

    // auto imageBegin = std::chrono::system_clock::now();
    // auto imageEnd = std::chrono::system_clock::now();
    // std::chrono::duration<double> imageDuration = imageEnd - imageBegin;

    // std::cout << "postgres time " << postgresQueryDuration.count() << "sec; "
    //     << "mds time " << mdsRequestDuration.count() << "sec; "
    //     << "image time " << imageDuration.count() << "sec" << std::endl;


    // res = {
    //   "type": "FeatureCollection",
    //   "features": [
    //     {
    //       "type": "Feature",
    //       "id": x + y * 2**20 + z * 2**40,
    //       "geometry": {
    //         "type": "Point",
    //         "coordinates": [point.y, point.x]
    //       },
    //       "properties": {
    //         "balloonContent": "content",
    //         "clusterCaption": "label 1",
    //         "hintContent": "Hint text"
    //       }
    //     }
    //   ]
    // }

    // double x = 37.61718;


    mj::Builder builder;
    builder << [](mj::ObjectBuilder b) {
        b["type"] = "FeatureCollection";
        b["features"] = [](mj::ArrayBuilder b) {
            b << [](mj::ObjectBuilder b) {
                b["type"] = "Feature";
                b["id"] = 1;
                b["geometry"] = [](mj::ObjectBuilder b) {
                    b["type"] = "Point";
                    b["coordinates"] = [](mj::ArrayBuilder b) {
                        b << 55.75744;
                        b << 37.61718;
                    };
                };
                b["properties"] = [](mj::ObjectBuilder b) {
                    b["balloonContent"] = "content";
                    b["clusterCaption"] = "lable 1";
                    b["hintContent"] = "Hint text";
                };
            };
        };
    };

    response["Content-Type"] = "application/json";
    if (callbackOpt) {
        response << *callbackOpt << "(" << builder.str() << ")";
    } else {
        response << builder.str();
    }
}

int main(int /*argc*/, const char** /*argv*/)
{

    mpgp::PoolConfigurationPtr poolConfiguration(mpgp::PoolConfiguration::create());

    poolConfiguration->master = mpgp::InstanceId{"pg94.maps.dev.yandex.net", 5432};
    mpgp::PoolConstantsPtr poolConstants(mpgp::PoolConstants::create(32, 32, 0, 0));
    poolConstants->getTimeoutMs = 5000;
    poolConstants->pingIntervalMs = 5000;

    PgpoolLogger logger;
    g_poolPtr.reset(new mpgp::Pool(
        logger,
        std::move(poolConfiguration),
        "dbname=mapspro_production user=mapspro password=mapspro",
        std::move(poolConstants)
    ));

    INFO() << "Initializing";
    yacare::run();
    INFO() << "Shutting down";
    return 0;
}
