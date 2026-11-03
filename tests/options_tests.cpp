#include "app/options.h"
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>
using br::app::Options;
static Options parse(std::initializer_list<const char*> args) {
    std::vector<std::string> storage{"Backrooms"};
    for(auto arg:args)storage.emplace_back(arg);
    std::vector<char*> argv;for(auto& arg:storage)argv.push_back(arg.data());
    return br::app::parseOptions(int(argv.size()),argv.data());
}
static void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
static void rejects(std::initializer_list<const char*> args){
    bool threw=false;try{parse(args);}catch(const std::exception&){threw=true;}
    check(threw,"Malformed diagnostic options must be rejected");
}
int main(){
    auto timed=parse({"--hidden","--run-seconds","600"});
    check(timed.runSeconds==600&&timed.frames==0,"Timed soak must not inherit hidden 60-frame default");
    auto stress=parse({"--hidden","--stress-cycles","9"});
    check(stress.frames==3360,"Stress defaults must allow every transition to settle");
    auto combined=parse({"--hidden","--stress-cycles","3","--run-seconds","600"});
    check(combined.frames==0,"Wall-clock deadline controls combined stress/soak");
    check(parse({"--hidden"}).frames==60,"Ordinary hidden smoke behavior preserved");
    check(parse({"--hidden","--run-seconds","600","--frames","12"}).frames==12,"Explicit frame limit remains respected");
    rejects({"--run-seconds","2junk"});rejects({"--stress-cycles","1.9"});
    rejects({"--run-seconds","nan"});rejects({"--slow-frame-ms","inf"});
    rejects({"--run-seconds","-1"});rejects({"--stress-cycles","1001"});
    rejects({"--stress-interval","39"});rejects({"--slow-frame-ms","0"});
    std::cout<<"Diagnostic CLI tests passed.\n";
}
