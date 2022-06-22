# Any-data stack with canary and hash protection

## Description & Features
This project includes the description of the stack itself and a bunch of useful methods for working with it. These methods include:
- Constructor and destructor
- Push and pop
- Verification
- Stack dump

This stack is automatically resized (performing hysteresis to avoid *"getting stuck"* on pushing and poping one exact element). It can store any type of data (all type-dependent functions use *void \**) - integer numbers, floating-point numbers, structures, pointers etc.

In *src/stack.hpp* header file you can find some definitions related to safety modes. **STACK_VERIFICATION_ON** enables automatic stack parameters' diagnostics upon every stack function being used. If any issues are detected, the function will return an error code and info about **all** the flaws found will be dumped to *LOG.txt* file with a bunch of useful information about stack's condition and the whole stack's stored data itself. It slows stack's work down a bit, but makes it much safer. Beyond this, with this definition commented out other 2 will be automatically disabled regardless to whether you commented them out or not. Manual verifier function (returning a number with info about **all** the flaws binary encoded in it) will be available in any case. Manual dump does not provide information about possible flaws (as it is a separate function from the verifier), but shows everything else. **STACK_CANARY_DEFENSE_ON** enables standard canary defense of stack's structure itself and its data storage. Canaries' condition will be checked during every automatic stack's diagnostics - this protects it from *silent* spoilage because of most possible *external* memory errors (array overflows, mainly). **STACK_HASH_DEFENSE_ON** works in the same way, but enables automatic hashing and rehashing (in case the state has legally changed) of stack's structure and data storage. Obviously, it also implies regular (just as regular as other automatic verifications) hash checking. This actually provides defense from almost any type of memory-damaging incidents. Canary and hash defense can be enabled and disabled independently. Manual verification implies canary and hash checks if they are requested.

## Latest version
The latest version of the cyclic list can be found here: <https://github.com/quaiion/overprotected-stack>.

## Dependencies
No special software besides compiler + linker is required.

## Documentation
In-code documentation (compatible with Doxygen) will be added soon.

## License
This is an open-source educational project. You can freely use, edit and distribute any versions of this code in personal and commercial purposes.

## Contacts
#### Korney Ivanishin, author of the project
email: <korney059@gmail.com>,
GitHub: [quaiion](https://github.com/quaiion)
