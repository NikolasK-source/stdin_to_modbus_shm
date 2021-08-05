#pragma once

#include <string>

#include <string>
class SHM {
private:
    std::string name;
    int         fd   = -1;
    std::size_t size = 0;
    void *      addr = nullptr;

public:
    explicit SHM(std::string name);

    ~SHM();

    /*! \brief get size of the shared memory object
     *
     * @return size in bytes
     */
    [[nodiscard]] size_t get_size() const { return size; }

    /*! \brief get address to the shared memory
     *
     * caller is responsible for selecting the right element type!
     *
     * @tparam type element type
     * @return address
     */
    template <typename type = void *>
    [[nodiscard]] type get_addr() {
        static_assert(std::is_pointer<type>::value, "Expected a pointer type");
        return reinterpret_cast<type>(addr);
    }

    /*! \brief get name of the shared memory object
     *
     * @return name
     */
    [[nodiscard]] const std::string &get_name() const { return name; }
};
