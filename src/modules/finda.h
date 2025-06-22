#pragma once

namespace modules {
namespace finda {

struct FINDA {
    void BlockingInit() {}        // chiamata in main()
    void Step() {}                // chiamata in main()
    bool Pressed() const { return false; }
};

extern FINDA finda;

}  // namespace finda
}  // namespace modules
