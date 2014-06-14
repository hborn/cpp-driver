#pragma once
#include "test_utils.hpp"

struct policy_tool {

  std::map<CassInet, int> coordinators;

  void show_coordinators();	// show what queries went to what node IP.
  void reset_coordinators();
  
  int
  init(
       CassSession* session,
       int n,
       CassConsistency cl,
       bool batch = false);

  void
  create_schema(
                CassSession* session,
                int replicationFactor);

  void
  add_coordinator(CassInet coord_addr);

  void
  assertQueried(
                CassInet coord_addr,
                int n);

  void
  assertQueriedAtLeast(
                       CassInet coord_addr,
                       int n);

  int
  query(
        CassSession* session,
        int n,
        CassConsistency cl);
};