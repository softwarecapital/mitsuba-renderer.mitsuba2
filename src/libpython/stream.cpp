#include <mitsuba/core/stream.h>
#include <mitsuba/core/astream.h>
#include <mitsuba/core/dstream.h>
#include <mitsuba/core/fstream.h>
#include <mitsuba/core/mstream.h>

#include <mitsuba/core/filesystem.h>
#include "python.h"

NAMESPACE_BEGIN()

// User provides a templated [T] function that takes arguments (which might depend on T)
struct declare_stream_accessors {
    using PyClass = pybind11::class_<mitsuba::Stream,
                                     mitsuba::ref<mitsuba::Stream>>;

    template <typename T>
    static void apply(PyClass &c) {
        c.def("write", [](Stream& s, const T &value) {
            s.write(value);
        }, DM(Stream, write, 2));
    }
};
struct declare_astream_accessors {
    using PyClass = pybind11::class_<mitsuba::AnnotatedStream,
                                     mitsuba::ref<mitsuba::AnnotatedStream>>;

    template <typename T>
    static void apply(PyClass &c) {
        c.def("get", [](AnnotatedStream& s,
                        const std::string &name, T &value) {
            return py::cast(s.get(name, value));
        }, DM(AnnotatedStream, get));
        c.def("set", [](AnnotatedStream& s,
                        const std::string &name, const T &value) {
            s.set(name, value);
        }, DM(AnnotatedStream, set));
    }
};

/// Use this type alias to list the supported types. Be wary of automatic type conversions.
// TODO: support all supported types that can occur in Python
// TODO: Python `long` type probably depends on the architecture, test on 32bits
using methods_declarator = for_each_type<bool, int64_t, Float, std::string>;

NAMESPACE_END()

MTS_PY_EXPORT(Stream) {
#define DECLARE_READ(Type, ReadableName) \
    def("read" ReadableName, [](Stream& s) {     \
        Type v;                                  \
        s.read(v);                               \
        return py::cast(v);                      \
    }, DM(Stream, read, 2))

    auto c = MTS_PY_CLASS(Stream, Object)
        .mdef(Stream, canWrite)
        .mdef(Stream, canRead)
        .mdef(Stream, setByteOrder)
        .mdef(Stream, getByteOrder)
        .mdef(Stream, getHostByteOrder)
        .DECLARE_READ(int64_t, "Long")
        .DECLARE_READ(Float, "Float")
        .DECLARE_READ(bool, "Boolean")
        .DECLARE_READ(std::string, "String")
        // TODO: support lists and dicts?
        .def("__repr__", &Stream::toString);
#undef DECLARE_READ

    methods_declarator::recurse<declare_stream_accessors>(c);

    py::enum_<Stream::EByteOrder>(c, "EByteOrder", DM(Stream, EByteOrder))
        .value("EBigEndian", Stream::EBigEndian)
        .value("ELittleEndian", Stream::ELittleEndian)
        .value("ENetworkByteOrder", Stream::ENetworkByteOrder)
        .export_values();
}

MTS_PY_EXPORT(DummyStream) {
    MTS_PY_CLASS(DummyStream, Stream)
        .def(py::init<>(), DM(DummyStream, DummyStream))
        .mdef(DummyStream, seek)
        .mdef(DummyStream, truncate)
        .mdef(DummyStream, getPos)
        .mdef(DummyStream, getSize)
        .mdef(DummyStream, flush)
        .mdef(DummyStream, canWrite)
        .mdef(DummyStream, canRead)
        .mdef(DummyStream, canRead)
        .def("__repr__", &DummyStream::toString);
}

MTS_PY_EXPORT(FileStream) {
    MTS_PY_CLASS(FileStream, Stream)
        .def(py::init<const mitsuba::filesystem::path &, bool>(), DM(FileStream, FileStream))
        .mdef(FileStream, seek)
        .mdef(FileStream, truncate)
        .mdef(FileStream, getPos)
        .mdef(FileStream, getSize)
        .mdef(FileStream, flush)
        .def("__repr__", &FileStream::toString);
}

MTS_PY_EXPORT(MemoryStream) {
    MTS_PY_CLASS(MemoryStream, Stream)
        .def(py::init<size_t>(), DM(MemoryStream, MemoryStream),
             py::arg("initialSize") = 512)
        .mdef(MemoryStream, seek)
        .mdef(MemoryStream, truncate)
        .mdef(MemoryStream, getPos)
        .mdef(MemoryStream, getSize)
        .mdef(MemoryStream, flush)
        .mdef(MemoryStream, canRead)
        .mdef(MemoryStream, canWrite)
        .def("__repr__", &MemoryStream::toString);
}

MTS_PY_EXPORT(AnnotatedStream) {
    auto c = MTS_PY_CLASS(AnnotatedStream, Object)
        .def(py::init<ref<Stream>, bool>(), DM(AnnotatedStream, AnnotatedStream),
             py::arg("stream"), py::arg("throwOnMissing") = true)
        .mdef(AnnotatedStream, close)
        .mdef(AnnotatedStream, push)
        .mdef(AnnotatedStream, pop)
        .mdef(AnnotatedStream, keys)
        .mdef(AnnotatedStream, getSize)
        .mdef(AnnotatedStream, canRead)
        .mdef(AnnotatedStream, canWrite)
        .def("__repr__", &AnnotatedStream::toString);

    // get & set declarations for many types
    // TODO: read & set methods à la dict (see Properties bindings)
    // TODO: infer type from type info stored in the ToC map
    methods_declarator::recurse<declare_astream_accessors>(c);
}
