# CS2SchemaGen

CS2SchemaGen is a CS2 plugin to generate Source 2 SDKs. When used with CS2, it adds a concommand `schema_dump_all` that dumps the [Source 2 schemas](https://praydog.com/reverse-engineering/2015/06/24/source2.html) in JSON form.

## Getting Started

These instructions will help you set up the project on your local machine for development and testing purposes.

### Prerequisites

- Visual Studio 2019 or newer
- premake5

### Clone the repository

To clone the repository with submodules, run the following command:

```bash
git clone --recurse-submodules https://github.com/saul/cs2schemagen.git
```

### Building the project

1. Open a command prompt or terminal in the project's root directory.
1. Run the following command to generate the Visual Studio solution:
   ```bash
   premake5 vs2019
   ```
1. Open the generated source2gen.sln file in Visual Studio.
1. Build the solution in the desired configuration (Debug, Release, or Dist).

## Credits

This project is based upon [neverlossec/source2gen](https://github.com/neverlosecc/source2gen), which is the joint effort of various individuals/projects. Special thanks to the following:

- **[es3n1n](https://github.com/es3n1n)** - source2gen [contributor](https://github.com/neverlosecc/source2gen/commits?author=es3n1n)
- **[cpz](https://github.com/cpz)** - source2gen [contributor](https://github.com/neverlosecc/source2gen/commits?author=cpz)
- **[Soufiw](https://github.com/Soufiw)** - source2gen [contributor](https://github.com/neverlosecc/source2gen/commits?author=Soufiw)
- **[anarh1st47](https://github.com/anarh1st47)** - source2gen [contributor](https://github.com/neverlosecc/source2gen/commits?author=anarh1st47)
- **[praydog](https://github.com/praydog)** - the author of the original [Source2Gen](https://github.com/praydog/Source2Gen) project

This project also utilizes the following open-source libraries:

- **[Premake](https://github.com/premake/premake-core)** - Build configuration tool
