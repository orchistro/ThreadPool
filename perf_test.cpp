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
static void test_body(const size_t aPoolSize, std::vector<size_t>* aWhiteBoardVector, const std::vector<size_t>& aUniqueIdList)
{
    TP sPool(aPoolSize);

    for (const auto sId : aUniqueIdList)
    {
        (void)sPool.push(task, aWhiteBoardVector, sId);
    }

    sPool.stop();
}

int32_t main(const int32_t aArgc, const char* aArgv[])
{
    size_t sCnt = std::atoll(aArgv[1]);
    size_t sPoolSize = std::atoll(aArgv[2]);
    std::string sMethod{aArgv[3]};

    std::vector<size_t> sWhiteBoardVector(sCnt, 0L);    // prepare sCnt 0-initialized vector
    std::vector<size_t> sIdList{gen_unique_distrubitud_vector(sCnt)};

    if (sMethod == "lambda")
    {
        test_body<ThreadPoolLambda>(sPoolSize, &sWhiteBoardVector, sIdList);
    }
    else if (sMethod == "async")
    {
        test_body<ThreadPoolAsync>(sPoolSize, &sWhiteBoardVector, sIdList);
    }

    if (check_white_board(sWhiteBoardVector) == true)
    {
        std::cout << "PASS: All integers in the map are equally increased\n";
    }

    return 0;
}
