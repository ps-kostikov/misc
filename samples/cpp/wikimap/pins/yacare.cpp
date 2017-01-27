#include <yandex/maps/yacare.h>
#include <yandex/maps/pgpool3/pgpool3.h>
#include <yandex/maps/json/builder.h>
#include <yandex/maps/json/value.h>
#include <yandex/maps/tileutils5/tile.h>

#include <boost/optional.hpp>

#include <chrono>
#include <iostream>
#include <sstream>

namespace mpgp = maps::pgpool3;
namespace mj = maps::json;
namespace mtu = maps::tileutils5;

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

std::string mercatorWkt(uint64_t x, uint64_t y, uint64_t z)
{
    mtu::Tile tile(x, y, z);
    auto mBox = tile.mercatorBox();
    std::stringstream ss;
    ss << "POLYGON ((";
    ss << mBox.lt().x() << " " << mBox.lt().y() << ", ";
    ss << mBox.rb().x() << " " << mBox.lt().y() << ", ";
    ss << mBox.rb().x() << " " << mBox.rb().y() << ", ";
    ss << mBox.lt().x() << " " << mBox.rb().y() << ", ";
    ss << mBox.lt().x() << " " << mBox.lt().y();
    ss << "))";
    return ss.str();
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

    auto mWkt = mercatorWkt(*xOpt, *yOpt, *zOpt);
    // std::cout << mWkt << std::endl;

    auto connection = g_poolPtr->getMasterConnection();
    auto transaction = mpgp::makeWriteableTransaction(std::move(connection));
    std::stringstream query;
    query << "select id, comment, st_x(position) as x, st_y(position) as y from pkostikov.pins_all ";
    query << "where st_within(position, st_transform(st_setsrid(st_geomfromtext('" << mWkt<< "'), 3395), 4326))";
    auto postgresQueryBegin = std::chrono::system_clock::now();
    auto result = transaction->exec(query.str());
    auto postgresQueryEnd = std::chrono::system_clock::now();
    std::chrono::duration<double> postgresQueryDuration = postgresQueryEnd - postgresQueryBegin;

    // if (result.empty()) {
    //     std::cout << "postgres time " << postgresQueryDuration.count() << "sec" << std::endl;
    //     throw yacare::errors::NotFound() << "no db record";
    // }

// GET /signals-renderer?x=74533&y=45031&z=17

    std::cout << "postgres time " << postgresQueryDuration.count() << "sec" << std::endl;
    

    // double x = 37.61718;


    mj::Builder builder;
    builder << [&result](mj::ObjectBuilder b) {
        b["type"] = "FeatureCollection";
        b["features"] = [&result](mj::ArrayBuilder b) {
            for (const auto& row: result) {
                b << [&row](mj::ObjectBuilder b) {
                    b["type"] = "Feature";
                    b["id"] = row["id"].as<uint64_t>();
                    b["geometry"] = [&row](mj::ObjectBuilder b) {
                        b["type"] = "Point";
                        b["coordinates"] = [&row](mj::ArrayBuilder b) {
                            b << row["y"].as<double>();
                            b << row["x"].as<double>();
                        };
                    };
                    b["properties"] = [&row](mj::ObjectBuilder b) {
                        b["balloonContent"] = row["comment"].as<std::string>();
                        b["clusterCaption"] = "lable 1";
                        b["hintContent"] = row["comment"].as<std::string>();
                    };
                };
            }
        };
    };

    response["Content-Type"] = "application/json; charset=utf-8";
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
