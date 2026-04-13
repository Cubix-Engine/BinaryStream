#ifndef BINARYSTREAM_HPP
#define BINARYSTREAM_HPP

#include <bit>
#include <expected>
#include <iomanip>
#include <optional>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace cubix {
    class BinaryStreamException : public std::runtime_error {
    public:
        enum class Error {
            OutOfBounds,
            MalformedVarInt,
            MalformedVarLong,
            InvalidStringLength
        };

    private:
        Error  mType{};
        size_t mPosition{};

    public:
        explicit BinaryStreamException(
            const Error type, const size_t position = 0, const std::string_view message = ""
        )
            : std::runtime_error(makeMessage(type, position, message)), mType(type),
              mPosition(position) {};

        [[nodiscard]] Error type() const noexcept {
            return mType;
        }
        [[nodiscard]] size_t position() const noexcept {
            return mPosition;
        }

        static BinaryStreamException outOfBounds(const size_t pos) {
            return BinaryStreamException{Error::OutOfBounds, pos};
        }

        static std::string_view toString(const Error type) {
            switch (type) {
            case Error::OutOfBounds:
                return "OutOfBounds";
            case Error::MalformedVarInt:
                return "MalformedVarInt";
            case Error::MalformedVarLong:
                return "MalformedVarLong";
            case Error::InvalidStringLength:
                return "InvalidStringLength";
            }

            return "Unknown";
        }

    private:
        static std::string
        makeMessage(const Error type, size_t position, std::string_view message = "") {
            if (message.empty()) {
                return std::format("BinaryStream: {} at {}", toString(type), position);
            }

            return std::format("BinaryStream: {} at {}: {}", toString(type), position, message);
        }
    };

    // BinaryStream stores data in a std::vector.
    // Any write may reallocate and invalidate spans returned by readBytes().
    class BinaryStream {
    public:
        using Error = BinaryStreamException::Error;

        template <typename T, std::endian E>
        struct serialize {
            static std::expected<T, BinaryStreamException> read(BinaryStream& stream) {
                if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>) {
                    return stream.tryRead<T, E>();
                }
                else {
                    static_assert(
                        sizeof(T) == 0,
                        "BinaryStream::serialize<T>::read not implemented for this type"
                    );
                }
            }

            static void write(const T& value, BinaryStream& stream) {
                if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>) {
                    T v = BinaryStream::adjustEndian<T, E>(value);

                    stream.writeBytes(BinaryStream::asBytes(v));
                }
                else {
                    static_assert(
                        sizeof(T) == 0,
                        "BinaryStream::serialize<T>::write not implemented for this type"
                    );
                }
            }
        };

    private:
        std::vector<uint8_t> mStream{};
        size_t               mReadPos{0};

    public:
        BinaryStream() = default;
        explicit BinaryStream(const std::vector<uint8_t>& data) : mStream(data) {};
        explicit BinaryStream(std::vector<uint8_t>&& data) noexcept
            : mStream(std::move(data)) {};

        [[nodiscard]] const uint8_t* data() const {
            return this->mStream.data();
        }

        void reset() {
            this->mStream.clear();
            this->mReadPos = 0;
        }

        [[nodiscard]] const std::vector<uint8_t>& vector() const {
            return mStream;
        }
        [[nodiscard]] std::span<const uint8_t> span() const {
            return {mStream.data(), mStream.size()};
        }

        void reserve(const size_t size) {
            mStream.reserve(size);
        }

        [[nodiscard]] BinaryStream sliceCopy(const size_t length) {
            auto bytes = readBytes(length);

            return BinaryStream(std::vector(bytes.begin(), bytes.end()));
        }

        [[nodiscard]] size_t size() const {
            return this->mStream.size();
        }

        [[nodiscard]] bool eof() const noexcept {
            return mReadPos >= mStream.size();
        }

        [[nodiscard]] size_t bytesLeft() const noexcept {
            return mStream.size() - mReadPos;
        }

        [[nodiscard]] bool canRead(const size_t n) const noexcept {
            return bytesLeft() >= n;
        }

        [[nodiscard]] size_t position() const noexcept {
            return mReadPos;
        }

        void seek(const size_t pos) {
            if (pos > mStream.size()) {
                throw std::out_of_range("Seek out of bounds");
            }

            mReadPos = pos;
        }

        template <typename T, std::endian E = std::endian::little>
        std::expected<T, BinaryStreamException> peek() noexcept {
            const auto pos   = mReadPos;
            auto       value = tryRead<T, E>();

            mReadPos = pos;
            return value;
        }

        void skip(const size_t length = 1) {
            if (length > this->bytesLeft()) {
                throw std::out_of_range("Read past end");
            }

            this->mReadPos += length;
        }

        void align(const size_t alignment) {
            if (alignment == 0) {
                return;
            }

            if (const size_t mis = mReadPos % alignment) {
                skip(alignment - mis);
            }
        }

        // Readers
        [[nodiscard]] uint8_t readUint8() {
            return this->read<uint8_t>();
        }

        [[nodiscard]] int8_t readInt8() {
            const auto value = this->readUint8();
            return static_cast<int8_t>(value);
        }

        [[nodiscard]] std::expected<std::span<const uint8_t>, BinaryStreamException>
        tryReadBytes(const size_t length) noexcept {
            if (!canRead(length)) {
                return std::unexpected(
                    BinaryStreamException{
                        Error::OutOfBounds, mReadPos, "Not enough bytes to read"
                    }
                );
            }

            const size_t pos  = mReadPos;
            mReadPos         += length;

            return std::span<const uint8_t>(mStream.data() + pos, length);
        }

        // NOTE: Returned span references internal buffer.
        // Any write operation may reallocate and invalidate it.
        [[nodiscard]] std::span<const uint8_t> readBytes(const size_t length) {
            return unwrap(this->tryReadBytes(length));
        }

        template <typename T, std::endian E = std::endian::little>
        [[nodiscard]] std::expected<T, BinaryStreamException> tryRead() noexcept {
            static_assert(
                std::is_trivially_copyable_v<T>,
                "BinaryStream::tryRead requires trivially copyable type"
            );

            if (!canRead(sizeof(T))) {
                return std::unexpected(
                    BinaryStreamException{
                        Error::OutOfBounds, mReadPos, "Not enough bytes to read value"
                    }
                );
            }

            T value;
            std::memcpy(&value, mStream.data() + mReadPos, sizeof(T));
            mReadPos += sizeof(T);

            return adjustEndian<T, E>(value);
        }

        template <typename T, std::endian E = std::endian::little>
        [[nodiscard]] T read() {
            return unwrap(serialize<T, E>::read(*this));
        }

        [[nodiscard]] std::expected<uint32_t, BinaryStreamException>
        tryReadVarUint32() noexcept {
            if (bytesLeft() < 1) {
                return std::unexpected(
                    BinaryStreamException{
                        Error::OutOfBounds, mReadPos, "Unexpected end while reading VarInt"
                    }
                );
            }

            uint32_t value = 0;
            int      shift = 0;

            // We can safely read up to 5 bytes max
            const size_t maxReadable = std::min(bytesLeft(), static_cast<size_t>(5));

            for (size_t i = 0; i < maxReadable; ++i) {
                const uint8_t byte = mStream[mReadPos++];

                value |= (static_cast<uint32_t>(byte & 0x7F) << shift);

                if ((byte & 0x80) == 0) [[likely]] {
                    return value;
                }

                shift += 7;
            }

            // If we consumed all allowed bytes and still not finished:
            if (maxReadable < 5) {
                return std::unexpected(
                    BinaryStreamException{
                        Error::OutOfBounds, mReadPos, "Unexpected end while reading VarInt"
                    }
                );
            }

            return std::unexpected(
                BinaryStreamException{Error::MalformedVarInt, mReadPos, "VarInt too large"}
            );
        }

        [[nodiscard]] uint32_t readVarUint32() {
            return unwrap(this->tryReadVarUint32());
        }

        [[nodiscard]] std::expected<int32_t, BinaryStreamException> tryReadVarInt32() {
            const auto raw = this->tryReadVarUint32();
            if (!raw) {
                return std::unexpected(raw.error());
            }

            return static_cast<int32_t>(*raw >> 1) ^ -static_cast<int32_t>(*raw & 1);
        }

        [[nodiscard]] int32_t readVarInt32() {
            return unwrap(this->tryReadVarInt32());
        }

        template <std::endian E = std::endian::little>
        [[nodiscard]] uint32_t readUint32() {
            return this->read<uint32_t, E>();
        }

        template <std::endian E = std::endian::little>
        [[nodiscard]] int32_t readInt32() {
            const uint32_t raw = this->readUint32<E>();
            return static_cast<int32_t>(raw);
        }

        template <std::endian E = std::endian::little>
        [[nodiscard]] uint16_t readUint16() {
            return this->read<uint16_t, E>();
        }

        template <std::endian E = std::endian::little>
        [[nodiscard]] int16_t readInt16() {
            const uint16_t raw = this->readUint16<E>();
            return static_cast<int16_t>(raw);
        }

        [[nodiscard]] std::expected<uint64_t, BinaryStreamException>
        tryReadVarUint64() noexcept {
            if (bytesLeft() < 1) {
                return std::unexpected(
                    BinaryStreamException{
                        Error::OutOfBounds, mReadPos, "Unexpected end while reading VarLong"
                    }
                );
            }

            uint64_t value = 0;
            int      shift = 0;

            const size_t maxReadable = std::min(bytesLeft(), static_cast<size_t>(10));

            for (size_t i = 0; i < maxReadable; ++i) {
                const uint8_t byte = mStream[mReadPos++];

                // Stricter check: The 10th byte of a VarLong can only hold 1 bit.
                if (i == 9 && (byte & ~0x01)) {
                    return std::unexpected(
                        BinaryStreamException{
                            Error::MalformedVarLong, mReadPos, "VarLong overflow"
                        }
                    );
                }

                value |= (static_cast<uint64_t>(byte & 0x7F) << shift);

                if ((byte & 0x80) == 0) [[likely]]
                    return value;

                shift += 7;
            }

            if (maxReadable < 10) {
                return std::unexpected(
                    BinaryStreamException{
                        Error::OutOfBounds, mReadPos, "Unexpected end while reading VarLong"
                    }
                );
            }

            return std::unexpected(
                BinaryStreamException{Error::MalformedVarLong, mReadPos, "VarLong too large"}
            );
        }

        [[nodiscard]] uint64_t readVarUint64() {
            return unwrap(this->tryReadVarUint64());
        }

        [[nodiscard]] std::expected<int64_t, BinaryStreamException> tryReadVarInt64() {
            const auto raw = this->tryReadVarUint64();
            if (!raw) {
                return std::unexpected(raw.error());
            }

            return static_cast<int64_t>(*raw >> 1) ^ -static_cast<int64_t>(*raw & 1);
        }

        [[nodiscard]] int64_t readVarInt64() {
            return unwrap(this->tryReadVarInt64());
        }

        template <std::endian E = std::endian::little>
        [[nodiscard]] uint64_t readUint64() {
            return this->read<uint64_t, E>();
        }

        template <std::endian E = std::endian::little>
        [[nodiscard]] int64_t readInt64() {
            const uint64_t raw = this->readUint64<E>();
            return static_cast<int64_t>(raw);
        }

        [[nodiscard]] std::expected<std::string, BinaryStreamException>
        tryReadString(const size_t length) noexcept {
            auto bytes = tryReadBytes(length);
            if (!bytes) {
                return std::unexpected(bytes.error());
            }

            return std::string(reinterpret_cast<const char*>(bytes->data()), bytes->size());
        }

        template <std::endian E = std::endian::little>
        [[nodiscard]] std::expected<std::string, BinaryStreamException> tryReadString() {
            auto length = tryRead<uint32_t, E>();
            if (!length) {
                return std::unexpected(length.error());
            }

            return this->tryReadString(*length);
        }

        template <std::endian E = std::endian::little>
        [[nodiscard]] std::string readString() {
            return unwrap(this->tryReadString<E>());
        }

        [[nodiscard]] std::string readString(const size_t length) {
            return unwrap(this->tryReadString(length));
        }

        [[nodiscard]] bool readBoolean() {
            return this->readUint8() != 0;
        }

        // Writers
        void writeBytes(std::span<const uint8_t> bytes) {
            if (const size_t newSize = mStream.size() + bytes.size();
                newSize > mStream.capacity()) {
                mStream.reserve(newSize);
            }

            mStream.insert(mStream.end(), bytes.begin(), bytes.end());
        }

        void writeBytes(const uint8_t* data, const size_t length) {
            if (!data && length > 0) {
                throw std::invalid_argument("BinaryStream::writeBytes: null pointer");
            }

            if (length > 0) {
                writeBytes(std::span(data, length));
            }
        }

        template <typename T, std::endian E = std::endian::little>
        void write(const T& value) {
            serialize<T, E>::write(value, *this);
        }

        void writeUint8(const uint8_t value) {
            this->write<uint8_t>(value);
        }

        void writeInt8(const int8_t value) {
            this->writeUint8(static_cast<uint8_t>(value));
        }

        void writeVarUint32(uint32_t value) {
            for (int i = 0; i < 5; ++i) {
                const uint8_t toWrite   = value & 0x7f;
                value                 >>= 7;

                if (value != 0) [[likely]] {
                    this->writeUint8(toWrite | 0x80);
                    continue;
                }

                this->writeUint8(toWrite);
                break;
            }
        }

        void writeVarInt32(const int32_t value) {
            this->writeVarUint32(
                ((static_cast<uint32_t>(value) << 1) ^ static_cast<uint32_t>(value >> 31)) &
                0xFFFFFFFFL
            );
        }

        template <std::endian E = std::endian::little>
        void writeUint32(const uint32_t value) {
            this->write<uint32_t, E>(value);
        }

        template <std::endian E = std::endian::little>
        void writeInt32(const int32_t value) {
            this->writeUint32<E>(static_cast<uint32_t>(value));
        }

        template <std::endian E = std::endian::little>
        void writeUint16(const uint16_t value) {
            this->write<uint16_t, E>(value);
        }

        template <std::endian E = std::endian::little>
        void writeInt16(const int16_t value) {
            this->writeUint16<E>(static_cast<uint16_t>(value));
        }

        void writeVarUint64(uint64_t value) {
            for (int i = 0; i < 10; ++i) {
                const uint8_t toWrite   = value & 0x7f;
                value                 >>= 7;

                if (value != 0) [[likely]] {
                    this->writeUint8(toWrite | 0x80);
                    continue;
                }

                this->writeUint8(toWrite);
                break;
            }
        }

        void writeVarInt64(const int64_t value) {
            this->writeVarUint64(
                (static_cast<uint64_t>(value) << 1) ^ static_cast<uint64_t>(value >> 63)
            );
        }

        template <std::endian E = std::endian::little>
        void writeUint64(const uint64_t value) {
            this->write<uint64_t, E>(value);
        }

        template <std::endian E = std::endian::little>
        void writeInt64(const int64_t value) {
            this->writeUint64<E>(static_cast<uint64_t>(value));
        }

        void writeBoolean(const bool value) {
            this->writeUint8(value ? 1 : 0);
        }

        template <typename T>
        void writeString(const std::string& value, const T length) {
            if (value.size() >= length) {
                this->writeBytes(reinterpret_cast<const uint8_t*>(value.data()), length);
            }
            else {
                this->writeBytes(reinterpret_cast<const uint8_t*>(value.data()), value.size());

                const size_t padLength = length - value.size();
                mStream.insert(mStream.end(), padLength, 0);
            }
        }

        template <std::endian E = std::endian::little>
        void writeString(const std::string& value) {
            this->writeUint32<E>(static_cast<uint32_t>(value.size()));
            this->writeBytes(reinterpret_cast<const uint8_t*>(value.data()), value.size());
        }

        template <typename T, std::endian E>
        [[nodiscard]] static constexpr T adjustEndian(T value) noexcept {
            static_assert(
                std::is_integral_v<T> || std::is_floating_point_v<T>,
                "BinaryStream::adjustEndian requires primitive type"
            );

            static_assert(
                sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8,
                "BinaryStream::adjustEndian Unsupported type size for endian conversion"
            );

            if constexpr (E == std::endian::native || sizeof(T) == 1) {
                return value;
            }

            if constexpr (std::is_integral_v<T>) {
                return std::byteswap(value);
            }
            else {
                using U = std::conditional_t<sizeof(T) == 4, uint32_t, uint64_t>;
                return std::bit_cast<T>(std::byteswap(std::bit_cast<U>(value)));
            }
        }

        template <typename T>
        static std::span<const uint8_t> asBytes(const T& value) {
            static_assert(std::is_trivially_copyable_v<T>);

            return {reinterpret_cast<const uint8_t*>(&value), sizeof(T)};
        }

    private:
        template <typename T>
        [[nodiscard]] static T unwrap(std::expected<T, BinaryStreamException>&& result) {
            if (!result) {
                throw BinaryStreamException(std::move(result.error()));
            }

            return *result;
        }
    };

    template <typename T, std::endian E>
    struct cubix::BinaryStream::serialize<std::vector<T>, E> {

        static std::expected<std::vector<T>, BinaryStreamException>
        read(cubix::BinaryStream& stream) {

            return stream.tryRead<uint32_t, E>().and_then(
                [&](auto size) -> std::expected<std::vector<T>, BinaryStreamException> {
                    std::vector<T> result;

                    if constexpr (std::is_trivially_copyable_v<T>) {
                        if (size > stream.bytesLeft() / sizeof(T)) {
                            return std::unexpected(
                                BinaryStreamException{
                                    Error::OutOfBounds, stream.position(),
                                    "Vector size too large"
                                }
                            );
                        }

                        auto bytes = stream.tryReadBytes(size * sizeof(T));
                        if (!bytes) {
                            return std::unexpected(bytes.error());
                        }

                        result.resize(size);
                        std::memcpy(result.data(), bytes->data(), bytes->size());

                        if constexpr (E != std::endian::native) {
                            for (auto& value : result) {
                                value = BinaryStream::adjustEndian<T, E>(value);
                            }
                        }
                    }
                    else {
                        result.reserve(size);

                        for (uint32_t i = 0; i < size; ++i) {
                            auto value = serialize<T, E>::read(stream);
                            if (!value) {
                                return std::unexpected(value.error());
                            }

                            result.emplace_back(*value);
                        }
                    }

                    return result;
                }
            );
        }

        static void write(const std::vector<T>& vec, cubix::BinaryStream& stream) {
            stream.write<uint32_t, E>(static_cast<uint32_t>(vec.size()));

            for (const auto& value : vec) {
                stream.write<T, E>(value);
            }
        }
    };

    template <typename T, std::endian E>
    struct cubix::BinaryStream::serialize<std::optional<T>, E> {

        static std::expected<std::optional<T>, BinaryStreamException>
        read(cubix::BinaryStream& stream) {

            auto hasValue = stream.tryRead<bool>();
            if (!hasValue) {
                return std::unexpected(hasValue.error());
            }

            if (!*hasValue) {
                return std::optional<T>{};
            }

            auto value = serialize<T, E>::read(stream);
            if (!value) {
                return std::unexpected(value.error());
            }

            return std::optional<T>(*value);
        }

        static void write(const std::optional<T>& value, cubix::BinaryStream& stream) {
            stream.write<bool>(value.has_value());

            if (value.has_value()) {
                stream.write<T, E>(*value);
            }
        }
    };

    template <std::endian E>
    struct cubix::BinaryStream::serialize<std::string, E> {

        static std::expected<std::string, BinaryStreamException> read(BinaryStream& stream) {

            return stream.tryRead<uint32_t, E>()
                .and_then([&](auto length) { return stream.tryReadBytes(length); })
                .transform([](auto bytes) {
                    return std::string(
                        reinterpret_cast<const char*>(bytes.data()), bytes.size()
                    );
                });
        }

        static void write(const std::string& value, BinaryStream& stream) {
            stream.write<uint32_t, E>(static_cast<uint32_t>(value.size()));
            stream.writeBytes(reinterpret_cast<const uint8_t*>(value.data()), value.size());
        }
    };
} // namespace cubix

#endif // !BINARYSTREAM_HPP