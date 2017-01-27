#include <yandex/maps/yacare.h>
#include <yandex/maps/pgpool3/pgpool3.h>
#include <yandex/maps/json/builder.h>
#include <yandex/maps/json/value.h>
#include <yandex/maps/tileutils5/tile.h>

#include <boost/optional.hpp>

#include <chrono>
#include <iostream>
#include <sstream>
#include <vector>

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

struct Feedback
{
    uint64_t id;
    std::string comment;
    double x;
    double y;
};

typedef std::vector<Feedback> Feedbacks;

struct Pin
{
    std::string id;
    std::string balloonContent;
    std::string clusterCaption;
    std::string hintContent;
    double x;
    double y;
};
typedef std::vector<Pin> Pins;

Pins convertToPins(const Feedbacks& feedbacks)
{
    Pins result;
    for (const auto& feedback: feedbacks) {
        Pin pin;
        pin.id = std::to_string(feedback.id);
        pin.balloonContent = feedback.comment;
        pin.hintContent = feedback.comment;
        pin.x = feedback.x;
        pin.y = feedback.y;
        result.push_back(pin);
    }
    return result;
}

Pins squashToPins(const Feedbacks& feedbacks)
{
    if (feedbacks.size() <= 1) {
        return convertToPins(feedbacks);
    }
    Pin pin;

    pin.id = "";
    for (const auto& f: feedbacks) {
        pin.id += "_";
        pin.id += std::to_string(f.id);
        if (pin.id.size() > 1000) {
            break;
        }
    }

    pin.balloonContent = std::to_string(feedbacks.size());
    pin.hintContent = std::to_string(feedbacks.size());

    pin.x = 0.;
    for (const auto& f: feedbacks) {
        pin.x += f.x;
    }
    pin.x /= feedbacks.size();

    pin.y = 0.;
    for (const auto& f: feedbacks) {
        pin.y += f.y;
    }
    pin.y /= feedbacks.size();

    return {pin};
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

    Feedbacks feedbacks;
    for (const auto& row: result) {
        Feedback feedback;
        feedback.id = row["id"].as<uint64_t>();
        feedback.comment = row["comment"].as<std::string>();
        feedback.x = row["x"].as<double>();
        feedback.y = row["y"].as<double>();
        feedbacks.push_back(feedback);
    }

    auto pins = squashToPins(feedbacks);
    // auto pins = convertToPins(feedbacks);

    // if (result.empty()) {
    //     std::cout << "postgres time " << postgresQueryDuration.count() << "sec" << std::endl;
    //     throw yacare::errors::NotFound() << "no db record";
    // }

// GET /signals-renderer?x=74533&y=45031&z=17

    std::cout << "postgres time " << postgresQueryDuration.count() << "sec" << std::endl;
    

    // double x = 37.61718;


    mj::Builder builder;
    builder << [&pins](mj::ObjectBuilder b) {
        b["type"] = "FeatureCollection";
        b["features"] = [&pins](mj::ArrayBuilder b) {
            for (const auto& pin: pins) {
                b << [&pin](mj::ObjectBuilder b) {
                    b["type"] = "Feature";
                    b["id"] = pin.id;
                    b["geometry"] = [&pin](mj::ObjectBuilder b) {
                        b["type"] = "Point";
                        b["coordinates"] = [&pin](mj::ArrayBuilder b) {
                            b << pin.y;
                            b << pin.x;
                        };
                    };
                    b["properties"] = [&pin](mj::ObjectBuilder b) {
                        b["balloonContent"] = pin.balloonContent;
                        b["clusterCaption"] = pin.clusterCaption;
                        b["hintContent"] = pin.hintContent;
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
