<?php

require "tests.php";
require "director_exception.php";

// No new functions
check::functions(array(foo_ping,foo_pong,launder,bar_ping,bar_pong,bar_pang));
// No new classes
check::classes(array(director_exception,Foo,Exception1,Exception2,Base,Bar));
// now new vars
check::globals(array());

class MyException extends Exception {
  function __construct($a, $b) {
    $this->msg = $a . $b;
  }
}

class MyFoo extends Foo {
  function ping() {
    throw new Exception("MyFoo::ping() EXCEPTION");
  }
}

class MyFoo2 extends Foo {
  function ping() {
    return true;
  }
}

class MyFoo3 extends Foo {
  function ping() {
    throw new MyException("foo", "bar");
  }
}

# Check that the Exception raised by MyFoo.ping() is returned by 
# MyFoo.pong().
$ok = 0;
$a = new MyFoo();
$b = launder($a);
try {
  $b->pong();
} catch (Exception $e) {
  $ok = 1;
  check::equal($e->getMessage(), "MyFoo::ping() EXCEPTION", "Unexpected error message #1");
}
check::equal($ok, 1, "Got no exception while expected one #1");

# Check that the director returns an Exception if the return type is 
# wrong.
$ok = 0;
$a = new MyFoo2();
$b = launder($a);
try {
  $b->pong();
} catch (Exception $e) {
  $ok = 1;
  check::equal($e->getMessage(), "Swig director type mismatch in output value of type 'std::string'", "Unexpected error message #2");
}
check::equal($ok, 1, "Got no exception while expected one #2");

# Check that the director can return an exception which requires two 
# arguments to the constructor, without mangling it.
$ok = 0;
$a = new MyFoo3();
$b = launder($a);
try {
  $b->pong();
} catch (Exception $e) {
  $ok = 1;
  check::equal($e->getMessage(), "foobar", "Unexpected error message #3");
}
check::equal($ok, 1, "Got no exception while expected one #3");

try {
  throw new Exception2();
} catch (Exception2 $e2) {
}

try {
  throw new Exception1();
} catch (Exception1 $e1) {
}

check::done();
?>
