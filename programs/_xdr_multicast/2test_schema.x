namespace test;

const BASE = 0x200;

enum data_types {
   TELEM = BASE + 1
};

enum Commands {
   TELEM = BASE + 1
};

struct Telem {
   int foo {
      name "John";
      key bar;
      unit "units";
      description "This is a longer test";
   };
   unsigned hyper bar {
      name "Test 2";
   };
} = data_types::TELEM;

command "test-telem" {
   summary "sets the process' telemetry struct";
   param data_types::TELEM;
} = Commands::TELEM;




