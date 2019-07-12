namespace test;

const BASE = 0x200;

enum data_types {
   STATUS = BASE + 1,
   PTEST = BASE + 2,
   MCAST = BASE + 3,
};

enum Commands {
   STATUS = BASE + 1,
   PTEST = BASE + 2,
   MCAST_TEST = BASE + 3,
};

struct Status {
   int foo {
      name "John";
      key bar;
      unit "units";
      description "This is a longer test";
   };
   unsigned hyper bar {
      name "Test 2";
   };
} = data_types::STATUS;

struct PTest {
   int val1 {
      key param1;
      description "The first test value";
   };
   int val2 {
      key val2;
      description "the second value";
   };
   int val3 {
      key polysat;
      description "Cal Poly Cubesat Lab";
   };
   int val4 {
      key test;
      description "a test value";
   };
} = data_types::PTEST;

struct Mcast {
   int val1 {
      key val1;
      name "Val 1";
      description "Value 1";
   };
   int val2 {
      key val2;
      name "Val 2";
      description "Value 2";
   };
} = data_types::MCAST;

command "test-status" {
   summary "Returns the process' status";
   response data_types::STATUS, data_types::PTEST;
} = IPC::CMDS::STATUS;

command "test-param" {
   summary "A command to use when testing command parameters";
   param data_types::PTEST;
} = Commands::PTEST;
