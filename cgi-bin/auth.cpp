#include "auth.h"

Auth auth() {
    return Auth{
        "db",         // Docker service name
        "root",       // Username
        "root",       // Password
        "quartermaster"// DB Name
    };
}