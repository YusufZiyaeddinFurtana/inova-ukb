#pragma once
#include "ek7.hpp"
// Change only after comparing official device vectors; no permissive auto-detection.
#ifndef INOVA_EK7_FLOAT_BIG
#define INOVA_EK7_FLOAT_BIG 0
#endif
#ifndef INOVA_EK7_CHECKSUM_FOOTERS
#define INOVA_EK7_CHECKSUM_FOOTERS 0
#endif
#ifndef INOVA_EK7_COMMAND_POLICY
#define INOVA_EK7_COMMAND_POLICY 0
#endif
#ifndef INOVA_EK7_INPUT_HZ
#define INOVA_EK7_INPUT_HZ 10
#endif
static_assert(INOVA_EK7_INPUT_HZ>=5 && INOVA_EK7_INPUT_HZ<=100,"Supported synthetic cadence: 5..100 Hz");
static_assert(INOVA_EK7_COMMAND_POLICY>=0 && INOVA_EK7_COMMAND_POLICY<=2,"0=table, 1=sum-prefix, 2=sum-with-footer");
inline inova::ek7::Profile ek7Profile() {
    inova::ek7::Profile p;
    p.floats=INOVA_EK7_FLOAT_BIG?inova::ek7::FloatOrder::Big:inova::ek7::FloatOrder::Little;
    p.checksum=INOVA_EK7_CHECKSUM_FOOTERS?inova::ek7::ChecksumScope::PrefixAndFooter:inova::ek7::ChecksumScope::Prefix;
    p.commands=static_cast<inova::ek7::CommandPolicy>(INOVA_EK7_COMMAND_POLICY);
    p.synthetic_period_s=1.0/INOVA_EK7_INPUT_HZ;return p;
}
