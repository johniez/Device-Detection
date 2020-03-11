Simple test of threading and data reloads.

Files representing c++ shared library:
  - `detection.h`
  - `detection.cc`

Source `test-simple.cc` runs simple detection in parallel and main thread
reloads the dataset each 200 ms.

When READ_LOCKING is set to zero, segfault occurs occasionally.
