#include <iostream>
#include <queue>
#include <set>
#include <string>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <algorithm>
#include <atomic>
#include <thread>

using namespace std;

/*

/getabc
tier1 : 10000
tier2: 1000
tier3 : 100

/putabc
tier1 : 100000
tier2 : 10000
tier3 : 1000
*/

using namespace std;

class APIConfig{
public:
    APIConfig(){}
    APIConfig(string apiName): apiName(apiName){}
    void setTierLimit(string tierName, int limit) {
        tierLimitMap[tierName] = limit;
    }

    int getTierLimit(string tierName) {
        if(tierLimitMap.count(tierName))
            return tierLimitMap[tierName];
        return 0;
    }

private:
    unordered_map<string, int>tierLimitMap;
    string apiName;
};

class APIConfigRegsitry{
public:
    APIConfigRegsitry(){

    }

    void AddConfigForEndPoint(string apiEndPoint, APIConfig config){
        configMap[apiEndPoint] = config;
    }
    APIConfig getConfigForEndPoint(string apiEndPoint) {
            return configMap[apiEndPoint];
    }
private:
    unordered_map<string, APIConfig> configMap;
};


APIConfigRegsitry gAPIConfigRegistry;

class RateLimiterService{
public:
    virtual void processRequest(string apiEndPoint, string tier) = 0;
private:
    virtual void fillTokens(string apiEndPoint, string tier) = 0;
};

class TokenBucketRateLimiterService : public RateLimiterService{
public:
    TokenBucketRateLimiterService() {}

    void processRequest(string apiEndPoint, string tier) override{
        fillTokens(apiEndPoint, tier);
        if(bucket > 0) {
            cout << "Forwarding request to service" << endl;
            bucket--;
        } else {
            cout << "Rejected request" << endl;
        }
    }

private:
    int getTokenLimitFromRegistry(string apiEndPoint, string tier) {
        APIConfig config = gAPIConfigRegistry.getConfigForEndPoint(apiEndPoint);
        return config.getTierLimit(tier);
    }

    void fillTokens(string apiEndPoint, string tier) override{
        auto curTime = chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = curTime - startTime;
        int timePassed = elapsed_seconds.count();
        startTime = chrono::system_clock::now();

        int tokenLimit = getTokenLimitFromRegistry(apiEndPoint, tier);

        bucket = bucket + (timePassed*tokenLimit)/timeUnit;
        if(bucket > tokenLimit)
            bucket = tokenLimit;
    }


private:
    chrono::time_point<chrono::system_clock> startTime;
    double timeUnit = 2;
    int bucket = 0;
};


int main(){

    // apiconfig for /getabc
    APIConfig apiConfig1;
    apiConfig1.setTierLimit("tier1", 20);
    apiConfig1.setTierLimit("tier2", 1000);
    apiConfig1.setTierLimit("tier3", 100);

    gAPIConfigRegistry.AddConfigForEndPoint("getabc", apiConfig1);

    RateLimiterService* service = new TokenBucketRateLimiterService();
    auto task = [&](){
        for(int i=0;i< 100;i++){
            service->processRequest("getabc", "tier1");
        }
        this_thread::sleep_for(chrono::seconds(5));
        for(int i=0;i< 100;i++){
            service->processRequest("getabc", "tier1");
        }
    };
    
    thread t(task);
    t.join();
    return 0;
}
