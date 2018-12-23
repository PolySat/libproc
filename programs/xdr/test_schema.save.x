namespace test;
const BASE = 0x200;

enum data_types {
   STATUS = BASE + 1
};

enum Commands {
   STATUS = BASE + 1,

};

struct Status {
   int foo {
      name "John";
      key bar;
      unit "units";
      description "This is a longer test";
      divisor 5;
      offset 0;
   };
   unsigned hyper bar {
      name "Test 2";
   };
} = data_types::STATUS;

command Status {
   name "test-status";
   summary "Reports the general health status of the test process";
} = IPC::COMMANDS::STATUS;

command DataRequest {
   name "test-datareq";
   summary "foo";
   param IPC::DataReq;
} = IPC::COMMANDS::DATA_REQ;
