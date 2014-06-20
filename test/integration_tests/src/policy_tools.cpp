#include "policy_tools.hpp"

#include <boost/test/test_tools.hpp>
#include <boost/test/debug.hpp>
#include <boost/thread.hpp>
#include <boost/format.hpp>

void
policy_tool::show_coordinators()	// show what queries went to what nodes IP.
{
    for(std::map<CassInet, int>::const_iterator p = coordinators.begin(); p != coordinators.end(); ++p)
    {
        std::cout << *(reinterpret_cast<const int32_t*>(p->first.address)) << " : " << p->second << std::endl;
    }
}

void
policy_tool::reset_coordinators()
{
    coordinators.clear();
}

void
policy_tool::create_schema(
              CassSession* session,
              int replicationFactor)
{
    test_utils::execute_query(session,
                              str(boost::format(test_utils::CREATE_KEYSPACE_SIMPLE_FORMAT) % test_utils::SIMPLE_KEYSPACE % replicationFactor));
    test_utils::execute_query(session, str(boost::format("USE %s") % test_utils::SIMPLE_KEYSPACE));
    test_utils::execute_query(session,
                              str(boost::format("CREATE TABLE %s (k int PRIMARY KEY, i int)") % test_utils::SIMPLE_TABLE));
}

int
policy_tool::init(
     CassSession* session,
     int n,
     CassConsistency cl,
     bool batch)
{
    std::string query_string = str(boost::format("INSERT INTO %s(k, i) VALUES (0, 0)") % test_utils::SIMPLE_TABLE);
    
    if(batch)
    {
        std::string bth;
        bth.append("BEGIN BATCH ");
        bth.append(str(boost::format("INSERT INTO %s(k, i) VALUES (0, 0)") % test_utils::SIMPLE_TABLE));
        bth.append(" APPLY BATCH");
        query_string = bth;
    }
    
    for (int i = 0; i < n; ++i)
    {
        test_utils::CassResultPtr result;
        test_utils::execute_query(session,
                                  query_string,
                                  &result,
                                  cl);
    }
    return 0;
}

void
policy_tool::add_coordinator(
                CassInet coord_addr)
{
    if(coordinators.count(coord_addr) == 0)
        coordinators.insert(std::make_pair(coord_addr, 1));
    else
        coordinators[coord_addr] += 1;
}

void
policy_tool::assertQueried(
              CassInet coord_addr,
              int n)
{
    if(coordinators.count(coord_addr)!=0)
        BOOST_REQUIRE(coordinators[coord_addr] == n);
    else
        BOOST_REQUIRE(n == 0);
}

void
policy_tool::assertQueriedAtLeast(
                     CassInet coord_addr,
                     int n)
{
    int queried = coordinators[coord_addr];
    BOOST_REQUIRE(queried >= n);
}

int
policy_tool::query(
      CassSession* session,
      int n,
      CassConsistency cl)
{
    for (int i = 0; i < n; ++i)
    {
        test_utils::CassStatementPtr statement(cass_statement_new(
                                                      cass_string_init(str(boost::format("SELECT * FROM %s WHERE k = 0") % test_utils::SIMPLE_TABLE).c_str()),
                                                      0, cl));
        test_utils::CassFuturePtr future(cass_session_execute(session, statement.get()));
        test_utils::wait_and_check_error(future.get());
        
        CassInet sender;
        cass_future_get_client(future.get(), &sender);
        add_coordinator(sender);
    }
    return 0;
}

