.. -*- mode: rst; -*-

.. code-block:: javascript

   "cluster":{
        "storage_wiggler": {
            "wiggle_server_ids":["0ccb4e0feddb55"],
            "wiggle_server_addresses": ["127.0.0.1"],
            "primary": { // primary DC storage wiggler stats
                // One StorageServer wiggle round is considered 'complete', when all StorageServers with creationTime < T are wiggled
               "last_round_start_datetime": "2022-04-02 00:05:05.123 +0000",
               "last_round_start_timestamp": 1648857905.123, // when did the latest round start
               "last_round_finish_datetime": "1970-01-01 00:00:00.000 +0000",
               "last_round_finish_timestamp": 0, // when did the latest finished round finish
               "smoothed_round_seconds": 1, // moving average duration of a wiggle round
               "finished_round": 1,
               // 1 wiggle step as 1 storage server is wiggled in the current round
               "last_wiggle_start_datetime": "2022-04-02 00:05:05.123 +0000",
               "last_wiggle_start_timestamp": 1648857905.123, // when did the latest wiggle step start
               "last_wiggle_finish_datetime": "1970-01-01 00:00:00.000 +0000",
               "last_wiggle_finish_timestamp": 0,
               "smoothed_wiggle_seconds": 1,
               "finished_wiggle": 1
            },
            "remote": { // remote DC storage wiggler stats
               "last_round_start_datetime": "2022-04-02 00:05:05.123 +0000",
               "last_round_start_timestamp": 1648857905.123,
               "last_round_finish_datetime": "1970-01-01 00:00:00.000 +0000",
               "last_round_finish_timestamp": 0,
               "smoothed_round_seconds": 1,
               "finished_round": 1,
               "last_wiggle_start_datetime": "2022-04-02 00:05:05.123 +0000",
               "last_wiggle_start_timestamp": 1648857905.123,
               "last_wiggle_finish_datetime": "1970-01-01 00:00:00.000 +0000",
               "last_wiggle_finish_timestamp": 0,
               "smoothed_wiggle_seconds": 1,
               "finished_wiggle": 1
            }
       },
      "layers":{
         "_valid":true,
         "_error":"some error description"
      },
      "processes":{
         "$map_key=zoneid":{
            "version":"3.0.0", // a process version will not be reported if it is not protocol-compatible; it will be absent from status
            "machine_id":"0ccb4e0feddb5583010f6b77d9d10ece",
            "locality":{ // This will contain any locality fields that are provided on the command line
                "$map_key=localityName":"value"
            },
            "class_source":{
               "$enum":[
                  "command_line",
                  "configure_auto",
                  "set_class"
               ]
            },
            "class_type":{
               "$enum":[
                  "unset",
                  "storage",
                  "transaction",
                  "resolution",
                  "stateless",
                  "commit_proxy",
                  "grv_proxy",
                  "master",
                  "test",
                  "storage_cache"
               ]
            },
            "degraded":true,
            "roles":[
               {
                  "query_queue_max":0,
                  "local_rate":0,
                  "input_bytes":{
                     "hz":0.0,
                     "counter":0,
                     "roughness":0.0
                  },
                  "stored_bytes":12341234,
                  "kvstore_used_bytes":12341234,
                  "kvstore_available_bytes":12341234,
                  "kvstore_free_bytes":12341234,
                  "kvstore_total_bytes":12341234,
                  "kvstore_total_size":12341234,
                  "kvstore_total_nodes":12341234,
                  "kvstore_inline_keys":12341234,
                  "durable_bytes":{
                     "hz":0.0,
                     "counter":0,
                     "roughness":0.0
                  },
                  "queue_disk_used_bytes":12341234,
                  "queue_disk_available_bytes":12341234,
                  "queue_disk_free_bytes":12341234,
                  "queue_disk_total_bytes":12341234,
                  "role":{
                     "$enum":[
                        "master",
                        "commit_proxy",
                        "grv_proxy",
                        "log",
                        "storage",
                        "resolver",
                        "cluster_controller",
                        "data_distributor",
                        "ratekeeper",
                        "storage_cache",
                        "router",
                        "coordinator"
                     ]
                  },
                  "tss":false,
                  "storage_metadata":{
                     "created_time_datetime":"1970-01-01 00:00:00.000 +0000",
                     "created_time_timestamp": 0,
                     "storage_engine":{
                     "$enum":[
                     "ssd",
                     "ssd-1",
                     "ssd-2",
                     "ssd-redwood-1",
                     "ssd-rocksdb-v1",
                     "ssd-sharded-rocksdb",
                     "memory",
                     "memory-1",
                     "memory-2",
                     "memory-radixtree-beta",
                     "unknown"
                     ]}
                  },
                  "rocksdb_metrics":{
                     "block_cache_hits":12341234,
                     "block_cache_misses":12341234,
                     "pending_compaction_bytes":12341234,
                     "memtable_bytes":12341234,
                     "sst_reader_bytes":12341234,
                     "block_cache_usage":12341234,
                     "block_cache_limit":12341234,
                     "throttled_commits":12341234,
                     "write_stall_microseconds":12341234
                  },
                  "data_version":12341234,
                  "durable_version":12341234,
                  "data_lag": {
                     "seconds":5.0,
                     "versions":12341234
                  },
                  "durability_lag": {
                     "seconds":5.0,
                     "versions":12341234
                  },
                  "id":"eb84471d68c12d1d26f692a50000003f",
                  "total_queries":{
                     "hz":0.0,
                     "counter":0,
                     "roughness":0.0
                  },
                  "finished_queries":{
                     "hz":0.0,
                     "counter":0,
                     "roughness":0.0
                  },
                  "low_priority_queries":{
                     "hz":0.0,
                     "counter":0,
                     "roughness":0.0
                  },
                  "bytes_queried":{
                     "hz":0.0,
                     "counter":0,
                     "roughness":0.0
                  },
                  "keys_queried":{
                     "hz":0.0,
                     "counter":0,
                     "roughness":0.0
                  },
                  "mutation_bytes":{
                     "hz":0.0,
                     "counter":0,
                     "roughness":0.0
                  },
                  "mutations":{
                     "hz":0.0,
                     "counter":0,
                     "roughness":0.0
                  },
                  "fetched_versions":{
                     "hz":0.0,
                     "counter":0,
                     "roughness":0.0
                  },
                  "fetches_from_logs":{
                     "hz":0.0,
                     "counter":0,
                     "roughness":0.0
                  },
                  "grv_latency_statistics":{ // GRV Latency metrics are grouped according to priority (currently batch or default).
                     "default":{
                         "count":0,
                         "min":0.0,
                         "max":0.0,
                         "median":0.0,
                         "mean":0.0,
                         "p25":0.0,
                         "p90":0.0,
                         "p95":0.0,
                         "p99":0.0,
                         "p99.9":0.0
                     },
                     "batch":{
                         "count":0,
                         "min":0.0,
                         "max":0.0,
                         "median":0.0,
                         "mean":0.0,
                         "p25":0.0,
                         "p90":0.0,
                         "p95":0.0,
                         "p99":0.0,
                         "p99.9":0.0
                     }
                  },
                  "read_latency_statistics":{
                     "count":0,
                     "min":0.0,
                     "max":0.0,
                     "median":0.0,
                     "mean":0.0,
                     "p25":0.0,
                     "p90":0.0,
                     "p95":0.0,
                     "p99":0.0,
                     "p99.9":0.0
                  },
                  "commit_latency_statistics":{
                     "count":0,
                     "min":0.0,
                     "max":0.0,
                     "median":0.0,
                     "mean":0.0,
                     "p25":0.0,
                     "p90":0.0,
                     "p95":0.0,
                     "p99":0.0,
                     "p99.9":0.0
                  },
                  "commit_batching_window_size":{
                     "count":0,
                     "min":0.0,
                     "max":0.0,
                     "median":0.0,
                     "mean":0.0,
                     "p25":0.0,
                     "p90":0.0,
                     "p95":0.0,
                     "p99":0.0,
                     "p99.9":0.0
                  },
                  "grv_latency_bands":{ // How many GRV requests belong to the latency (in seconds) band (e.g., How many requests belong to [0.01,0.1] latency band). The key is the upper bound of the band and the lower bound is the next smallest band (or 0, if none). Example: {0.01: 27, 0.1: 18, 1: 1, inf: 98,filtered: 10}, we have 18 requests in [0.01, 0.1) band.
                     "$map_key=upperBoundOfBand": 1
                  },
                  "read_latency_bands":{
                     "$map_key=upperBoundOfBand": 1
                  },
                  "commit_latency_bands":{
                     "$map_key=upperBoundOfBand": 1
                  },
                  "busiest_read_tag":{
                     "tag": "",
                     "fractional_cost": 0.0,
                     "estimated_cost":{
                        "hz":0.0
                     }
                  },
                  "busiest_write_tag":{
                     "tag": "",
                     "fractional_cost": 0.0,
                     "estimated_cost":{
                        "hz":0.0
                     }
                  }
               }
            ],
            "command_line":"-r simulation",
            "memory":{
               "available_bytes":0, // an estimate of the process' fair share of the memory available to fdbservers
               "limit_bytes":0, // memory limit per process
               "unused_allocated_memory":0,
               "used_bytes":0, // virtual memory size of the process
               "rss_bytes":0 // resident memory size of the process
            },
            "messages":[
               {
                  "time":12345.12312,
                  "type":"x",
                  "name":{ // when not limiting
                     "$enum":[
                        "file_open_error",
                        "incorrect_cluster_file_contents",
                        "trace_log_file_write_error",
                        "trace_log_could_not_create_file",
                        "trace_log_writer_thread_unresponsive",
                        "process_error",
                        "io_error",
                        "io_timeout",
                        "platform_error",
                        "storage_server_lagging",
                        "(other FDB error messages)"
                     ]
                  },
                  "raw_log_message":"<stuff/>",
                  "description":"abc"
               }
            ],
            "fault_domain":"0ccb4e0fdbdb5583010f6b77d9d10ece",
            "under_maintenance":true,
            "excluded":false,
            "address":"1.2.3.4:1234",
            "disk":{
               "free_bytes":3451233456234,
               "reads":{
                  "hz":0.0,
                  "counter":0,
                  "sectors":0
               },
               "busy":0.0, // from 0.0 (idle) to 1.0 (fully busy)
               "writes":{
                  "hz":0.0,
                  "counter":0,
                  "sectors":0
               },
               "total_bytes":123412341234
            },
            "uptime_seconds":1234.2345,
            "cpu":{
               "usage_cores":0.0 // average number of logical cores utilized by the process over the recent past; value may be > 1.0
            },
            "network":{
               "current_connections":0,
               "connections_established":{
                   "hz":0.0
               },
               "connections_closed":{
                   "hz":0.0
               },
               "connection_errors":{
                   "hz":0.0
               },
               "megabits_sent":{
                  "hz":0.0
               },
               "megabits_received":{
                  "hz":0.0
               },
               "tls_policy_failures":{
                  "hz":0.0
               }
            },
            "run_loop_busy":0.2 // fraction of time the run loop was busy
         }
      },
      "logs":[
         {
            "log_interfaces":[ // this field will be absent if a value has not been explicitly set
               {
                  "id":"7f8d623d0cb9966e",
                  "healthy":true,
                  "address":"1.2.3.4:1234"
               }
            ],
            "epoch":1,
            "current":false,
            "begin_version":23,
            "end_version":112315141,
            "possibly_losing_data":true,
            "log_replication_factor":3,
            "log_write_anti_quorum":0,
            "log_fault_tolerance":2,
            "remote_log_replication_factor":3,
            "remote_log_fault_tolerance":2,
            "satellite_log_replication_factor":3,
            "satellite_log_write_anti_quorum":0,
            "satellite_log_fault_tolerance":2
         }
      ],
      "fault_tolerance":{
         "max_zone_failures_without_losing_availability":0,
         "max_zone_failures_without_losing_data":0
      },
      "qos":{
         "worst_queue_bytes_log_server":460,
         "batch_performance_limited_by":{
            "reason_server_id":"7f8d623d0cb9966e",
            "reason_id":0,
            "name":{ // when not limiting
               "$enum":[
                  "workload",
                  "storage_server_write_queue_size",
                  "storage_server_write_bandwidth_mvcc",
                  "storage_server_readable_behind",
                  "log_server_mvcc_write_bandwidth",
                  "log_server_write_queue",
                  "storage_server_min_free_space",
                  "storage_server_min_free_space_ratio",
                  "log_server_min_free_space",
                  "log_server_min_free_space_ratio",
                  "storage_server_durability_lag",
                  "storage_server_list_fetch_failed",
                  "blob_worker_lag",
                  "blob_worker_missing"
               ]
            },
            "description":"The database is not being saturated by the workload."
         },
         "performance_limited_by":{
            "reason_server_id":"7f8d623d0cb9966e",
            "reason_id":0,
            "name":{ // when not limiting
               "$enum":[
                  "workload",
                  "storage_server_write_queue_size",
                  "storage_server_write_bandwidth_mvcc",
                  "storage_server_readable_behind",
                  "log_server_mvcc_write_bandwidth",
                  "log_server_write_queue",
                  "storage_server_min_free_space",
                  "storage_server_min_free_space_ratio",
                  "log_server_min_free_space",
                  "log_server_min_free_space_ratio",
                  "storage_server_durability_lag",
                  "storage_server_list_fetch_failed",
                  "blob_worker_lag",
                  "blob_worker_missing"
               ]
            },
            "description":"The database is not being saturated by the workload."
         },
         "batch_transactions_per_second_limit":0,
         "transactions_per_second_limit":0,
         "batch_released_transactions_per_second":0,
         "released_transactions_per_second":0,
         "throttled_tags":{
            "auto" : {
                "busy_read" : 0,
                "busy_write" : 0,
                "count" : 0,
                "recommended_only":0
            },
            "manual" : {
                "count" : 0
            }
         },
         "limiting_queue_bytes_storage_server":0,
         "worst_queue_bytes_storage_server":0,
         "limiting_data_lag_storage_server":{
            "versions":0,
            "seconds":0.0
         },
         "worst_data_lag_storage_server":{
            "versions":0,
            "seconds":0.0
         },
         "limiting_durability_lag_storage_server":{
            "versions":0,
            "seconds":0.0
         },
         "worst_durability_lag_storage_server":{
            "versions":0,
            "seconds":0.0
         }
      },
      "incompatible_connections":[
      ],
      "datacenter_lag":{
         "seconds":1.0,
         "versions":1000000
      },
      "active_tss_count":0,
      "degraded_processes":0,
      "database_available":true,
      "database_lock_state":{
         "locked":true,
         "lock_uid":"00000000000000000000000000000000" // Only present when database is locked
      },
      "generation":2,
      "latency_probe":{ // all measurements are based on running sample transactions
         "read_seconds":7, // time to perform a single read
         "immediate_priority_transaction_start_seconds":0.0, // time to start a sample transaction at system immediate priority
         "batch_priority_transaction_start_seconds":0.0, // time to start a sample transaction at batch priority
         "transaction_start_seconds":0.0, // time to start a sample transaction at normal priority
         "commit_seconds":0.02 // time to commit a sample transaction
      },
      "clients":{
         "count":1,
         "supported_versions":[
             {
                 "client_version":"3.0.0",
                 "connected_clients":[
                     {
                         "address":"127.0.0.1:9898",
                         "log_group":"default"
                     }
                 ],
                 "max_protocol_clients":[
                     {
                         "address":"127.0.0.1:9898",
                         "log_group":"default"
                     }
                 ],
                 "count" : 1,
                 "max_protocol_count" : 1,
                 "protocol_version" : "fdb00a400050001",
                 "source_version" : "9430e1127b4991cbc5ab2b17f41cfffa5de07e9d"
             }
         ]
      },
      "page_cache":{
         "log_hit_rate":0.5,
         "storage_hit_rate":0.5
      },
      "messages":[
         {
            "reasons":[
               {
                  "description":"Blah."
               }
            ],
            "unreachable_processes":[
               {
                  "address":"1.2.3.4:1234"
               }
            ],
            "name":{ // when not limiting
               "$enum":[
                  "unreachable_master_worker",
                  "unreachable_dataDistributor_worker",
                  "unreachable_ratekeeper_worker",
                  "unreadable_configuration",
                  "full_replication_timeout",
                  "client_issues",
                  "unreachable_processes",
                  "immediate_priority_transaction_start_probe_timeout",
                  "batch_priority_transaction_start_probe_timeout",
                  "transaction_start_probe_timeout",
                  "read_probe_timeout",
                  "commit_probe_timeout",
                  "storage_servers_error",
                  "status_incomplete",
                  "layer_status_incomplete",
                  "database_availability_timeout",
                  "consistencycheck_suspendkey_fetch_timeout",
                  "consistencycheck_disabled",
                  "duplicate_mutation_streams",
                  "duplicate_mutation_fetch_timeout",
                  "primary_dc_missing",
                  "fetch_primary_dc_timeout",
                  "fetch_storage_wiggler_stats_timeout",
                  "fetch_consistency_scan_info_timeout"
               ]
            },
            "issues":[
               {
                  "name":{ // when not limiting
                     "$enum":[
                        "incorrect_cluster_file_contents",
                        "trace_log_file_write_error",
                        "trace_log_could_not_create_file",
                        "trace_log_writer_thread_unresponsive"
                     ]
                  },
                  "description":"Cluster file contents do not match current cluster connection string. Verify cluster file is writable and has not been overwritten externally."
               }
            ],
            "description":"abc"
         }
      ],
      "recovery_state":{
         "seconds_since_last_recovered":1,
         "required_resolvers":1,
         "required_commit_proxies":1,
         "required_grv_proxies":1,
         "name":{ // "fully_recovered" is the healthy state; other states are normal to transition through but not to persist in
            "$enum":[
               "reading_coordinated_state",
               "locking_coordinated_state",
               "locking_old_transaction_servers",
               "reading_transaction_system_state",
               "configuration_missing",
               "configuration_never_created",
               "configuration_invalid",
               "recruiting_transaction_servers",
               "initializing_transaction_servers",
               "recovery_transaction",
               "writing_coordinated_state",
               "accepting_commits",
               "all_logs_recruited",
               "storage_recovered",
               "fully_recovered"
            ]
         },
         "required_logs":3,
         "missing_logs":"7f8d623d0cb9966e",
         "active_generations":1,
         "description":"Recovery complete."
      },
      "workload":{
         "operations":{
            "writes":{
               "hz":0.0,
               "counter":0,
               "roughness":0.0
            },
            "reads":{ // measures number of completed read requests
               "hz":0.0,
               "counter":0,
               "roughness":0.0
            },
            "read_requests":{ // measures number of incoming read requests
               "hz":0.0,
               "counter":0,
               "roughness":0.0
            },
             "low_priority_reads":{ // measures number of incoming low priority read requests
               "hz":0.0,
               "counter":0,
               "roughness":0.0
            },
            "location_requests":{ // measures number of outgoing key server location responses
               "hz":0.0,
               "counter":0,
               "roughness":0.0
            },
            "memory_errors":{ // measures number of (commit/grv)_proxy_memory_limit_exceeded errors
               "hz":0.0,
               "counter":0,
               "roughness":0.0
            }
         },
         "bytes":{ // measures number of logical bytes read/written (ignoring replication factor and overhead on disk). Perfectly spaced operations will have a roughness of 1.0. Randomly spaced (Poisson-distributed) operations will have a roughness of 2.0, with increased bunching resulting in increased values. Higher roughness can result in increased latency due to increased queuing.
            "written":{
               "hz":0.0,
               "counter":0,
               "roughness":0.0
            },
            "read":{
               "hz":0.0,
               "counter":0,
               "roughness":0.0
            }
         },
         "keys":{
            "read":{
               "hz":0.0,
               "counter":0,
               "roughness":0.0
            }
         },
         "transactions":{
            "started":{
               "hz":0.0,
               "counter":0,
               "roughness":0.0
            },
            "started_immediate_priority":{
               "hz":0.0,
               "counter":0,
               "roughness":0.0
            },
            "started_default_priority":{
               "hz":0.0,
               "counter":0,
               "roughness":0.0
            },
            "started_batch_priority":{
               "hz":0.0,
               "counter":0,
               "roughness":0.0
            },
            "conflicted":{
               "hz":0.0,
               "counter":0,
               "roughness":0.0
            },
            "rejected_for_queued_too_long":{
               "hz":0.0,
               "counter":0,
               "roughness":0.0
            },
            "committed":{
               "hz":0.0,
               "counter":0,
               "roughness":0.0
            }
         }
      },
      "cluster_controller_timestamp":1415650089,
      "protocol_version":"fdb00a400050001",
      "connection_string":"a:a@127.0.0.1:4000",
      "full_replication":true,
      "maintenance_zone":"0ccb4e0fdbdb5583010f6b77d9d10ece",
      "maintenance_seconds_remaining":1.0,
      "data_distribution_disabled_for_ss_failures":true,
      "data_distribution_disabled_for_rebalance":true,
      "data_distribution_disabled":true,
      "active_primary_dc":"pv",
      "bounce_impact":{
         "can_clean_bounce":true,
         "reason":""
      },
      "configuration":{
         "log_anti_quorum":0,
         "log_replicas":2,
         "log_replication_policy":"(zoneid^3x1)",
         "redundancy_mode":{
         "$enum":[
             "single",
             "double",
             "triple",
             "three_datacenter",
             "three_datacenter_fallback",
             "three_data_hall",
             "three_data_hall_fallback"
         ]},
         "regions":[{
         "datacenters":[{
             "id":"mr",
             "priority":1,
             "satellite":1,
             "satellite_logs":2
         }],
         "satellite_redundancy_mode":{
         "$enum":[
             "one_satellite_single",
             "one_satellite_double",
             "one_satellite_triple",
             "two_satellite_safe",
             "two_satellite_fast"
         ]},
         "satellite_log_replicas":1,
         "satellite_usable_dcs":1,
         "satellite_anti_quorum":0,
         "satellite_log_policy":"(zoneid^3x1)",
         "satellite_logs":2
         }],
         "remote_redundancy_mode":{
         "$enum":[
             "remote_default",
             "remote_single",
             "remote_double",
             "remote_triple",
             "remote_three_data_hall"
         ]},
         "remote_log_replicas":3,
         "remote_logs":5,
         "log_routers":10,
         "usable_regions":1,
         "repopulate_anti_quorum":1,
         "storage_replicas":1,
         "resolvers":1, // this field will be absent if a value has not been explicitly set
         "storage_replication_policy":"(zoneid^3x1)",
         "logs":2, // this field will be absent if a value has not been explicitly set
         "log_version":2,
         "log_engine":{
         "$enum":[
             "ssd",
             "ssd-1",
             "ssd-2",
             "ssd-redwood-1",
             "ssd-rocksdb-v1",
             "ssd-sharded-rocksdb",
             "memory",
             "memory-1",
             "memory-2",
             "memory-radixtree-beta"
         ]},
         "log_spill":1,
         "storage_engine":{
         "$enum":[
             "ssd",
             "ssd-1",
             "ssd-2",
             "ssd-redwood-1",
             "ssd-rocksdb-v1",
             "ssd-sharded-rocksdb",
             "memory",
             "memory-1",
             "memory-2",
             "memory-radixtree-beta"
         ]},
         "tss_count":1,
         "tss_storage_engine":{
         "$enum":[
             "ssd",
             "ssd-1",
             "ssd-2",
             "ssd-redwood-1",
             "ssd-rocksdb-v1",
             "ssd-sharded-rocksdb",
             "memory",
             "memory-1",
             "memory-2",
             "memory-radixtree-beta"
         ]},
         "coordinators_count":1,
         "excluded_servers":[
            {
               "address":"10.0.4.1",
               "locality":"locality_processid:e9816ca4a89ff64ddb7ba2a5ec10b75b"
            }
         ],
         "auto_commit_proxies":3,
         "auto_grv_proxies":1,
         "auto_resolvers":1,
         "auto_logs":3,
         "commit_proxies":5, // this field will be absent if a value has not been explicitly set
         "grv_proxies":1, // this field will be absent if a value has not been explicitly set
         "proxies":6, // this field will be absent if a value has not been explicitly set
         "backup_worker_enabled":1,
         "perpetual_storage_wiggle": 0,
         "perpetual_storage_wiggle_locality":"0",
         "perpetual_storage_wiggle_engine":{
         "$enum":[
             "ssd",
             "ssd-1",
             "ssd-2",
             "ssd-redwood-1",
             "ssd-rocksdb-v1",
             "ssd-sharded-rocksdb",
             "memory",
             "memory-1",
             "memory-2",
             "memory-radixtree-beta",
             "none"
         ]},
         "storage_migration_type":{
         "$enum":[
             "disabled",
             "gradual",
             "aggressive"
         ]},
         "tenant_mode": {
             "$enum":[
             "disabled",
             "optional_experimental",
             "required_experimental"
         ]},
         "encryption_at_rest_mode": {
             "$enum":[
             "disabled",
             "aes_256_ctr"
         ]}
      },
      "data":{
         "least_operating_space_bytes_log_server":0,
         "average_partition_size_bytes":0,
         "state":{
            "healthy":true,
            "min_replicas_remaining":0,
            "name":{ // when not limiting
               "$enum":[
                  "initializing",
                  "missing_data",
                  "healing",
                  "optimizing_team_collections",
                  "healthy_populating_region",
                  "healthy_repartitioning",
                  "healthy_removing_server",
                  "healthy_rebalancing",
                  "healthy"
               ]
            },
            "description":""
         },
         "least_operating_space_ratio_storage_server":0.1,
         "max_machine_failures_without_losing_availability":0,
         "total_disk_used_bytes":0,
         "total_kv_size_bytes":0, // estimated
         "system_kv_size_bytes":0, // estimated
         "partitions_count":2,
         "moving_data":{
            "total_written_bytes":0, // reset whenever data distributor is re-recruited
            "in_flight_bytes":0, // number of bytes currently being moved between storage servers
            "in_queue_bytes":0, // number of bytes in the data distributor queue that should be moved (but are not yet being transferred between storage servers)
            "highest_priority":0
         },
         "team_trackers":[
            {
                "primary":true,
                "in_flight_bytes":0,
                "unhealthy_servers":0,
                "state":{
                    "healthy":true,
                    "min_replicas_remaining":0,
                    "name":{ // when not limiting
                       "$enum":[
                          "initializing",
                          "missing_data",
                          "healing",
                          "optimizing_team_collections",
                          "healthy_populating_region",
                          "healthy_repartitioning",
                          "healthy_removing_server",
                          "healthy_rebalancing",
                          "healthy"
                       ]
                    },
                    "description":""
                }
            }
         ],
         "least_operating_space_bytes_storage_server":0,
         "max_machine_failures_without_losing_data":0
      },
      "machines":{
         "$map_key=machineid":{
            "network":{
               "megabits_sent":{
                  "hz":0.0
               },
               "megabits_received":{
                  "hz":0.0
               },
               "tcp_segments_retransmitted":{
                  "hz":0.0
               }
            },
            "memory":{
               "free_bytes":0, // an estimate of how many bytes are free to allocate to fdbservers without swapping
               "committed_bytes":0, // an estimate of the number of bytes of memory on the machine that are not free for allocation
               "total_bytes":0 // an estimate of total physical RAM
            },
            "contributing_workers":4,
            "datacenter_id":"6344abf1813eb05b",
            "excluded":false,
            "address":"1.2.3.4",
            "machine_id":"6344abf1813eb05b",
            "locality":{ // This will contain any locality fields that are provided on the command line
                "$map_key=localityName":"value"
            },
            "cpu":{
               "logical_core_utilization":0.4 // computed as cpu_seconds / elapsed_seconds; value may be capped at 0.5 due to hyper-threading
            }
         }
      },
      "metacluster" : {
         "cluster_type" : "management",         // management, data, or standalone
         "metacluster_name" : "metacluster1",
         "metacluster_id" : 12345,
         "data_cluster_name" : "data_cluster1", // data cluster only
         "data_cluster_id" : 12346,             // data cluster only
         "num_data_clusters": 10                // management cluster only
      },
      "tenants" : {
         "num_tenants" : 1, // on data cluster, local count; on management cluster, total metacluster count
         "num_tenant_groups" : 10,
         "tenant_group_capacity" : 20,
      },
      "idempotency_ids" : {
         "size_bytes" : 0, // An estimate of the current number of bytes used in the database to store idempotency ids.
         "expired_version" : 0, // The commit status of a transaction whose commit version could be <= expired_version can no longer be determined.
         "expired_age" : 0, // The age in seconds of expired_version.
         "oldest_id_version" : 0, // The version of the oldest idempotency id still stored in the database.
         "oldest_id_age" : 0 // The age in seconds of the oldest_id_version.
      },
      "version_epoch":{
         "enabled": true,
         "epoch": 0 // The version epoch, as an offset from the Unix epoch. This field will be excluded if enabled is false.
      }
   },
   "client":{
      "coordinators":{
         "coordinators":[
            {
               "reachable":true,
               "address":"127.0.0.1:4701",
               "protocol":"0fdb00b070010001"
            }
         ],
         "quorum_reachable":true
      },
      "database_status":{
         "available":true,
         "healthy":true
      },
      "messages":[
         {
            "name":{ // when not limiting
               "$enum":[
                  "inconsistent_cluster_file",
                  "unreachable_cluster_controller",
                  "no_cluster_controller",
                  "status_incomplete_client",
                  "status_incomplete_coordinators",
                  "status_incomplete_error",
                  "status_incomplete_timeout",
                  "status_incomplete_cluster",
                  "quorum_not_reachable"
               ]
            },
            "description":"The cluster file is not up to date."
         }
      ],
      "timestamp":1415650089,
      "cluster_file":{
         "path":"/etc/foundationdb/fdb.cluster",
         "up_to_date":true
      }
   }
