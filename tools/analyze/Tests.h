#pragma once

// Engine checks that don't need references; returns the process exit code.
int runTests();
int runAcidCheck(); // just the 303 row
int runArpSyncCheck(); // just the arp in a DAW
