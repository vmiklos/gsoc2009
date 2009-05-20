<?php

# This file illustrates the cross language polymorphism using directors.

require("example.php");

# Create an Caller instance

$caller = new Caller();

# Add a simple C++ callback.

print "Adding and calling a normal C++ callback\n";
print "----------------------------------------\n";

$callback = new Callback();
$callback->thisown = 0;
$caller->setCallback($callback);
$caller->call();
$caller->delCallback();

# All done.

print "php exit\n";

?>
