<div align="center">

  # BinaryStream
  A fast, modern, header-only C++ utility for reading and writing binary data with explicit endianness and safe error handling.

  [![C++][CPP_BADGE_URL]][CPP_URL]
</div>

---

## Features

- Primitives (integral and floating point)
- `std::string`
- `std::vector`
- `std::optional`
- VarInt / VarLong encoding

Built for performance-critical use cases like networking and custom file formats.

## Library Example
```cpp
cubix::BinaryStream stream{};

stream.writeUint32(42);
stream.writeString("Hello!");

stream.seek(0);

auto number = stream.readUint32();
auto text   = stream.readString();
```

## License
This project is open-source under the **MIT License**. Feel free to modify and contribute!

## Contributing
```shell
# Fork the repository on GitHub, then:
git clone https://github.com/<your-username>/BinaryStream.git

# Create a new branch
git checkout -b feature-branch

# Commit your changes
git commit -m "Added new feature"

# Push to GitHub
git push origin feature-branch

# Submit a Pull Request
```

<!-- BADGES -->
[CPP_BADGE_URL]: https://img.shields.io/badge/C++-23-%2300599C?style=flat-square&logo=cplusplus&logoColor=%2300599C&labelColor=white
[CPP_URL]: https://cplusplus.com
<!-- BADGES -->
