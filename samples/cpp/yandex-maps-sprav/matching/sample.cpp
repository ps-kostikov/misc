#include <yandex/maps/sprav/model/storages/revision.h>
#include <yandex/maps/sprav/common/config.h>
#include <yandex/maps/sprav/algorithms/datamodel/company.h>
#include <yandex/maps/sprav/algorithms/similarity/similarity.h>
#include <yandex/maps/sprav/algorithms/unification/email_unifier.h>
#include <yandex/maps/sprav/algorithms/unification/url_unifier.h>
#include <yandex/maps/sprav/algorithms/unification/address_unifier.h>

#include <yandex/maps/pgpool2/pgpool.h>
#include <yandex/maps/common/exception.h>


#include <iostream>
#include <string>


namespace sprav = maps::sprav;
namespace model = maps::sprav::model;
namespace algorithms = maps::sprav::algorithms;


namespace model2algorithms {

algorithms::AddressComponent convert(const model::AddressComponent o);
algorithms::Name convert(const model::CompanyName& o);
algorithms::Phone convert(const model::CompanyPhone& o);
algorithms::Url convert(const model::CompanyUrl& o);
algorithms::WorkInterval convert(const model::WorkInterval& e);
algorithms::Email convert(const model::CompanyEmail& o);


algorithms::LocalizedString convert(const model::LocalizedString& ls)
{
    return algorithms::LocalizedString{ls.value, ls.locale};
}

template<class FromType, class ToType>
std::vector<ToType> convert(const std::set<FromType>& s)
{
    std::vector<ToType> result;
    for (const auto& e: s) {
        result.push_back(convert(e));
    }
    return result;
}

template<class T>
T convert(const boost153::optional<T>& bo)
{
    if (bo) {
        return *bo;
    }
    return T();
}

algorithms::AddressComponent::Kind convert(const model::AddressComponent::Kind& e)
{
    switch (e) {
        case model::AddressComponent::Kind::Country: return algorithms::AddressComponent::Kind::Country;
        case model::AddressComponent::Kind::Province: return algorithms::AddressComponent::Kind::Province;
        case model::AddressComponent::Kind::Area: return algorithms::AddressComponent::Kind::Area;
        case model::AddressComponent::Kind::Locality: return algorithms::AddressComponent::Kind::Locality;
        case model::AddressComponent::Kind::District: return algorithms::AddressComponent::Kind::District;
        case model::AddressComponent::Kind::Street: return algorithms::AddressComponent::Kind::Street;
        case model::AddressComponent::Kind::House: return algorithms::AddressComponent::Kind::House;
        case model::AddressComponent::Kind::Route: return algorithms::AddressComponent::Kind::Route;
        case model::AddressComponent::Kind::Other: return algorithms::AddressComponent::Kind::Other;
        case model::AddressComponent::Kind::Home: return algorithms::AddressComponent::Kind::Other;
        default: throw maps::RuntimeError() << "unexpected enum value " << static_cast<int>(e);
    }
}

algorithms::Address::Precision convert(const model::Address::Precision& e)
{
    switch (e) {
        case model::Address::Precision::Exact: return algorithms::Address::Precision::Exact;
        case model::Address::Precision::Number: return algorithms::Address::Precision::Number;
        case model::Address::Precision::Near: return algorithms::Address::Precision::Near;
        case model::Address::Precision::Range: return algorithms::Address::Precision::Range;
        case model::Address::Precision::Street: return algorithms::Address::Precision::Range;
        case model::Address::Precision::Other: return algorithms::Address::Precision::Range;
        default: throw maps::RuntimeError() << "unexpected enum value " << static_cast<int>(e);
    }
}


algorithms::AddressComponent convert(const model::AddressComponent o)
{
    return algorithms::AddressComponent{
        convert(o.kind()),
        convert(o.name())
    };
}

algorithms::Address convert(model::Address& o) 
{
    return algorithms::Address{
        o.regionCode(),
        o.geoId(),
        convert(o.formatted()),
        o.pos(),
        o.boundingBox(),
        convert(o.precision()),
        convert<model::AddressComponent, algorithms::AddressComponent>(o.components()),
        o.postalCode()
    };
}

algorithms::Name::Type convert(const model::CompanyName::Type& e)
{
    switch (e) {
        case model::CompanyName::Type::Main: return algorithms::Name::Type::Official;
        case model::CompanyName::Type::Short: return algorithms::Name::Type::Anno;
        case model::CompanyName::Type::Legal: return algorithms::Name::Type::Official;
        case model::CompanyName::Type::Obsolete: return algorithms::Name::Type::Official;
        case model::CompanyName::Type::Synonym: return algorithms::Name::Type::Synonym;
        default: throw maps::RuntimeError() << "unexpected enum value " << static_cast<int>(e);
    }   
}

algorithms::Name convert(const model::CompanyName& o)
{
    return algorithms::Name{
        convert(o.value()),
        convert(o.type())
    };
}

algorithms::Phone::Type convert(const model::CompanyPhone::Type& e)
{
    switch (e) {
        case model::CompanyPhone::Type::Phone: return algorithms::Phone::Type::Phone;
        case model::CompanyPhone::Type::Fax: return algorithms::Phone::Type::Fax;
        case model::CompanyPhone::Type::PhoneFax: return algorithms::Phone::Type::PhoneFax;
        default: throw maps::RuntimeError() << "unexpected enum value " << static_cast<int>(e);
    }   
}

algorithms::Phone convert(const model::CompanyPhone& o)
{
    return algorithms::Phone{
        convert<std::string>(o.countryCode()),
        convert<std::string>(o.regionCode()),
        o.number(),
        o.ext(),
        convert(o.type()),
        convert<model::LocalizedString, algorithms::LocalizedString>(o.infos())
    };
}

algorithms::Url::Type convert(const model::CompanyUrl::Type& e)
{
    switch (e) {
        case model::CompanyUrl::Type::Main: return algorithms::Url::Type::Own;
        case model::CompanyUrl::Type::Alternative: return algorithms::Url::Type::Other;
        case model::CompanyUrl::Type::Mirror: return algorithms::Url::Type::Other;
        case model::CompanyUrl::Type::Social: return algorithms::Url::Type::Social;
        case model::CompanyUrl::Type::Attribution: return algorithms::Url::Type::Attribution;
        case model::CompanyUrl::Type::Showtimes: return algorithms::Url::Type::Other;
        case model::CompanyUrl::Type::Booking: return algorithms::Url::Type::Other;
        default: throw maps::RuntimeError() << "unexpected enum value " << static_cast<int>(e);
    }   
}

algorithms::Url convert(const model::CompanyUrl& o)
{
    auto result = algorithms::unifyUrl(o.value());
    result.setType(convert(o.type()));
    return result;
}

algorithms::WorkInterval::Day convert(const model::WorkInterval::Day& e)
{
    switch (e) {
        case model::WorkInterval::Day::Monday: return algorithms::WorkInterval::Day::Monday;
        case model::WorkInterval::Day::Tuesday: return algorithms::WorkInterval::Day::Tuesday;
        case model::WorkInterval::Day::Wednesday: return algorithms::WorkInterval::Day::Wednesday;
        case model::WorkInterval::Day::Thursday: return algorithms::WorkInterval::Day::Thursday;
        case model::WorkInterval::Day::Friday: return algorithms::WorkInterval::Day::Friday;
        case model::WorkInterval::Day::Saturday: return algorithms::WorkInterval::Day::Saturday;
        case model::WorkInterval::Day::Sunday: return algorithms::WorkInterval::Day::Sunday;
        case model::WorkInterval::Day::Weekdays: return algorithms::WorkInterval::Day::Weekdays;
        case model::WorkInterval::Day::Weekend: return algorithms::WorkInterval::Day::Weekend;
        case model::WorkInterval::Day::Everyday: return algorithms::WorkInterval::Day::Everyday;
        default: throw maps::RuntimeError() << "unexpected enum value " << static_cast<int>(e);
    }   
}

algorithms::WorkInterval convert(const model::WorkInterval& o)
{
    return algorithms::WorkInterval{
        convert(o.day()),
        o.timeMinutesBegin(),
        o.timeMinutesEnd()
    };
}

algorithms::Email convert(const model::CompanyEmail& o)
{
    return algorithms::unifyEmail(o.value());
}


algorithms::Company convert(const model::Company& o)
{
    // FIXME add rubrics and features
    if (!o.address()) {
        throw maps::RuntimeError() << "address should be specified";
    }
    return algorithms::Company{
        convert(*o.address()),
        convert<model::CompanyName, algorithms::Name>(o.names()),
        convert<model::CompanyPhone, algorithms::Phone>(o.phones()),
        convert<model::CompanyUrl, algorithms::Url>(o.urls()),
        convert<model::WorkInterval, algorithms::WorkInterval>(o.workIntervals()),
        convert<model::CompanyEmail, algorithms::Email>(o.emails()),
        {}/*rubrics*/,
        {}/*features*/
    };
}

} // namespace model2algorithms

struct ReadSessionHolder {
    ReadSessionHolder(maps::pgpool2::ConnectionMgr& connMgr) :
        transaction(connMgr.readTransaction()),
        storage(new model::RevisionStorage(*transaction)),
        session(storage, model::Session::CachingPolicy::CacheAll)
    {}

    maps::pgpool2::PgTransaction transaction;
    std::shared_ptr<model::RevisionStorage> storage;
    model::Session session;
};

std::vector<model::Company> getCandidates(const model::Company& company, model::Session& session)
{
    std::vector<model::search::Filter> filters;

    filters.push_back(model::search::CompanyPos.intersects(company.address()->pos(), 10000));

    if (company.address()) {
        filters.push_back(model::search::CompanyAddress.match(company.address()->formatted().value));
    }

    std::vector<model::ID> objectIds = session.loadCompanyIds(filters);
    std::vector<model::Company> result;
    for (auto id : objectIds) {
        auto company = session.loadCompany(id);
        result.push_back(company);
    }
    return result;
    
}

std::vector<model::Company> getExperimentalCompanies(model::Session& session)
{
    std::vector<model::search::Filter> filters;

    filters.push_back(model::search::CompanyPublishingStatus.equals(model::Company::PublishingStatus::Duplicate));

    std::vector<model::ID> objectIds = session.loadCompanyIds(filters, 0, 1);

    std::vector<model::Company> result;
    for (auto id : objectIds) {
        auto company = session.loadCompany(id);
        result.push_back(company);
    }
    return result;
}

int main()
{
    std::cout << "hello" << std::endl;

    auto address = algorithms::unifyAddress("");

    std::cout << "address unified" << std::endl;

    auto connectionManager =
        sprav::config::createConnectionManager(
            sprav::config::config(),
            sprav::config::ConnectionPoolProfile::Editor);

    ReadSessionHolder holder(*connectionManager);
    auto companies = getExperimentalCompanies(holder.session);
    // auto company = holder.session.loadCompany(39358302);
    for (auto company : companies) {
        auto algComp = model2algorithms::convert(company);
        auto candidates = getCandidates(company, holder.session);
        std::cout << company.address()->formatted().value << std::endl;
        std::cout << "len(candidates) = " << candidates.size() << std::endl;
        std::cout << algComp << std::endl;
        for (auto candidate : candidates) {
            auto algCand = model2algorithms::convert(candidate);
            auto sim = algorithms::calculateSimilarity(algComp, algCand);
            if (sim >= 0.1) {
                std::cout << "sim = " << sim << std::endl;
                std::cout << algCand << std::endl;
            }
        }
        // std::cout << (int)company.publishingStatus() << std::endl;
        // if (company.duplicateCompanyId()) {
        //     std::cout << (int)*company.duplicateCompanyId() << std::endl;
        // }
    }

    // auto algCompany = model2algorithms::convert(company);
    // auto sim = algorithms::calculateSimilarity(algCompany, algCompany);
    // std::cout << "sim = " << sim << std::endl;
    return 0;
}