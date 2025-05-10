#pragma once

// When configured for use, set of random names for controller. Actual name is picked using ESP32-S3's unique identity as a key to the name - means we can flash to multiple
// devices and they should all get decently unique names.
// Max 17 chars for a name (final name will be longer after adding automated bits to the end of the name for bluetooth variants)
// Anything longer, name wont fit on display we are using!
const char* DeviceNames[] = { "Ellen", "Holly", "Indy", "Smudge", "Peanut", "Claire", "Eleanor", "Evilyn", "Andy", "Toya", "Verity", "Laura", "Eagle",
                        "Kevin", "Sophie", "Carla", "Hannah", "Becky", "Trent", "Jean", "Sebastian", "Phil", "Colin", "Berry", "Bazil", "Anne",
                        "Bella", "Vivian", "Bunny", "Thomas", "Giles", "David", "John", "Penny", "Beverly", "Dannie", "Ginny", "Samantha", "Sam",
                        "Lisa", "Charlie", "Albert", "Shoe", "Stevie" };
int DeviceNamesCount = sizeof(DeviceNames) / sizeof(DeviceNames[0]);