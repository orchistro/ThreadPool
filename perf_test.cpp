/*
 * vim:ts=4:shiftwidth=4:et:cindent:fileencoding=utf-8:
 */

#include <cstdint>
#include <cstdlib>
#include <ctime>

#include <string>
#include <vector>
#include <map>
#include <random>

#include "ThreadPoolLambda.hpp"
#include "ThreadPoolAsync.hpp"


static std::vector<size_t> gen_unique_distrubitud_vector(const size_t aSize)
{
    std::vector<size_t> sVector;
    sVector.reserve(aSize);

    for (size_t i = 0; i < aSize; i++)
    {
        sVector.push_back(i);
    }

    std::random_device sDevice;
    std::mt19937 sGenerator(sDevice());

    std::shuffle(sVector.begin(), sVector.end(), sGenerator);

    return sVector;
}

static bool check_white_board(const std::vector<size_t>& aVector)
{
    for (size_t i = 0; i < aVector.size(); i++)
    {
        if (aVector[i] != 1)
        {
            std::cout << "FAIL: index:" << i << " value:" << aVector[i] << " <-- should be 1\n";
            return false;
        }
    }

    return true;
}

void task(std::vector<size_t>* aVector, const size_t aId)
{
    aVector->at(aId)++;
}

template <typename TP>
static void test_core(TP* aPool, std::vector<size_t>* aWhiteBoardVector, const std::vector<size_t>* aUniqueIdList, const size_t aStart, const size_t aCnt)
{
    std::cout << "test_core start:" << aStart << ",cnt:" << aCnt << "\n";
    for (size_t i = 0; i < aCnt; i++)
    {
        (void)aPool->push(task, aWhiteBoardVector, (*aUniqueIdList)[aStart + i]);
    }
    std::cout << "---> test_core start:" << aStart << ",cnt:" << aCnt << "\n";
}

template <typename TP>
static void test(const size_t aEnquerCnt, const bool aAffinity, const size_t aPoolSize, std::vector<size_t>* aWhiteBoardVector, const std::vector<size_t>& aUniqueIdList)
{
    TP sPool(aPoolSize, aAffinity);

    std::vector<std::thread> sThreadList;
    sThreadList.reserve(aEnquerCnt);

    size_t sStart = 0;
    size_t sUnit = aUniqueIdList.size() / aEnquerCnt;
    size_t sRemaining = aUniqueIdList.size() % aEnquerCnt;

    for (size_t i = 0; i < aEnquerCnt; i++)
    {
        size_t sCnt = (i == 0) ? sUnit + sRemaining : sUnit;
        sThreadList.emplace_back(test_core<TP>, &sPool, aWhiteBoardVector, &aUniqueIdList, sStart, sCnt);
        sStart += sCnt;
    }

    for (auto& sThr : sThreadList)
    {
        sThr.join();
    }

    std::cout << "Stopping pool\n";
    sPool.stop();
    std::cout << "Stopping pool done\n";
}

int32_t main(const int32_t aArgc, const char* aArgv[])
{
    if (aArgc == 1)
    {
        std::cout << "Usage: perf_test DATA_CNT ENQUERCNT POOLSIZE AFFINITY (lambda|async)\n";
        exit(0);
    }

    size_t sDataCnt = std::atoll(aArgv[1]);
    size_t sEnquerCnt = std::atoll(aArgv[2]);
    size_t sPoolSize = std::atoll(aArgv[3]);
    bool sAffinity = std::atoll(aArgv[4]);
    std::string sMethod{aArgv[5]};

    std::vector<size_t> sWhiteBoardVector(sDataCnt, 0L);    // prepare sDataCnt 0-initialized vector
    std::vector<size_t> sIdList{gen_unique_distrubitud_vector(sDataCnt)};

    if (sMethod == "lambda")
    {
        test<ThreadPoolLambda>(sEnquerCnt, sAffinity, sPoolSize, &sWhiteBoardVector, sIdList);
    }
    else if (sMethod == "async")
    {
        test<ThreadPoolAsync>(sEnquerCnt, sAffinity, sPoolSize, &sWhiteBoardVector, sIdList);
    }

    if (check_white_board(sWhiteBoardVector) == true)
    {
        std::cout << "PASS: All integers in the map are equally increased\n";
    }

    return 0;
}
