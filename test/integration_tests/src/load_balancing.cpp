#define BOOST_TEST_DYN_LINK
#ifdef STAND_ALONE
#   define BOOST_TEST_MODULE cassandra
#endif

#include <chrono>
#include <algorithm>
#include <future>

#include <boost/test/unit_test.hpp>
#include <boost/test/debug.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/cstdint.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>

#include "cassandra.h"
#include "test_utils.hpp"
#include "policy_tools.hpp"

#include "cql_ccm_bridge.hpp"

struct LBTests : test_utils::MultipleNodesTest {
    LBTests() : MultipleNodesTest(2, 0) { }
};

BOOST_FIXTURE_TEST_SUITE(load_balancing_tests, LBTests)

BOOST_AUTO_TEST_CASE(test_round_robin)
{
  // TODO: AFAIK, the new API provides no way to retrieve the target endpoint from query result.
  // It should be implemented prior to running this test.
  BOOST_FAIL("Unimplemented");
  
  test_utils::CassFuturePtr session_future(cass_cluster_connect(cluster));
  test_utils::wait_and_check_error(session_future.get());
  test_utils::CassSessionPtr session(cass_future_get_session(session_future.get()));

  test_utils::execute_query(session.get(), str(boost::format(test_utils::CREATE_KEYSPACE_SIMPLE_FORMAT)
                                         % test_utils::SIMPLE_KEYSPACE % "1"));

  test_utils::execute_query(session.get(), str(boost::format("USE %s") % test_utils::SIMPLE_KEYSPACE));
  
  policy_tool pt;
  pt.create_schema(session.get(), 1);
  
  pt.init(session.get(), 12, CASS_CONSISTENCY_ONE);
  pt.query(session.get(), 12, CASS_CONSISTENCY_ONE);
  
  CassInet host1 = test_utils::inet_v4_from_string(conf.ip_prefix() + "1");
  CassInet host2 = test_utils::inet_v4_from_string(conf.ip_prefix() + "2");
  
  pt.assertQueried(host1, 6);
  pt.assertQueried(host2, 6);
  
  pt.reset_coordinators();
  ccm->bootstrap(3);
  
  boost::this_thread::sleep(boost::posix_time::seconds(15)); // wait for node 3 to be up
  CassInet host3 = test_utils::inet_v4_from_string(conf.ip_prefix() + "3");
  
  pt.query(session.get(), 12, CASS_CONSISTENCY_ONE);
  
  pt.assertQueried(host1, 4);
  pt.assertQueried(host2, 4);
  pt.assertQueried(host3, 4);
  
  pt.reset_coordinators();
  ccm->decommission(1);
  boost::this_thread::sleep(boost::posix_time::seconds(15)); // wait for node 1 to be down
  
  pt.query(session.get(), 12, CASS_CONSISTENCY_ONE);
  
  pt.assertQueried(host2, 6);
  pt.assertQueried(host3, 6);
}

BOOST_AUTO_TEST_SUITE_END()
