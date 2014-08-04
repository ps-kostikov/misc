#include "config.h"

#include <yandex/maps/pgpool2/pgpool.h>
#include <yandex/maps/wiki/revision/revisionsgateway.h>


#include <algorithm>
#include <fstream>
#include <streambuf>


namespace pool = maps::pgpool2;
namespace rev = maps::wiki::revision;


int main(int /*argc*/, const char** argv)
{
    Config config(argv[1]);
    
    maps::pgpool2::ConnectionMgr connMgr(config.dbSettings);

    pool::PgTransaction tran = connMgr.readTransaction();
    rev::RevisionsGateway gtw(*tran);
    std::cout << "head commit id = " << gtw.headCommitId() << "\n";

    auto snapshot = gtw.snapshot(gtw.headCommitId());

    // auto rev = snapshot.objectRevision(70118);
    // if (rev) {
    //     for (auto& a : *(rev->data().attributes)) {
    //         std::cout << a.first << " : " << a.second << "\n";
    //     }
    // }

    rev::filters::BinaryFilterExpr filter(
        rev::filters::BinaryFilterExpr::And,
        new rev::filters::AttrFilterExpr(
            rev::filters::AttrFilterExpr::Defined,
            "cat:company", std::string()),
        new rev::filters::NegativeFilterExpr(
            new rev::filters::AttrFilterExpr(
                rev::filters::AttrFilterExpr::Defined,
                "company:is_chain", std::string())
        ));

    // rev::filters::True filter;

    auto revList = snapshot.revisionIdsByFilter(filter);
    std::cout << "revisionIdsByFilter length " << revList.size() << "\n";

    auto objrevList = gtw.reader().loadRevisions(revList);
    std::cout << "revisionIdsByFilter length " << objrevList.size() << "\n";

    size_t n = 3;
    auto end = std::next(objrevList.begin(), std::min(n, objrevList.size()));
    rev::Reader reader = gtw.reader();

    std::for_each(objrevList.begin(), end, [&](rev::ObjectRevision& oRev) {

        for (auto& a : *(oRev.data().attributes)) {
            std::cout << a.first << " : " << a.second << "\n";

        }
        std::cout << "\n";
    });

    return 0;
}
