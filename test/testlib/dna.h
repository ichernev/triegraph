#ifndef __TESTLIB_DNA_CONFIG_H__
#define __TESTLIB_DNA_CONFIG_H__

#include "manager.h"
#include "dna_config.h"

namespace test {
    // using namespace triegraph;
    // using namespace triegraph::dna;
    using triegraph::Manager;
    using triegraph::dna::DnaConfig;
    using triegraph::dna::CfgFlags;
    using DnaConfig_RK = DnaConfig<0, CfgFlags::VP_RAW_KMERS>;
    using Manager_RK = Manager<DnaConfig_RK>;
} /* namespace test */

#endif /* __TESTLIB_DNA_CONFIG_H__ */
