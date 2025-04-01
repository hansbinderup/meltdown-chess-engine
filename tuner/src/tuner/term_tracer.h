#pragma once

#ifdef TUNING

#include "evaluation/terms.h"

namespace evaluation {

template<size_t T>
using TraceTable = std::array<int16_t, T>;

#define TRACE(name, size) \
    TraceTable<size> name;

struct TermTracer {
    TERM_LIST(TRACE)
};

struct TraceInfo {
    std::string_view name;
    std::span<const int16_t> traces;
};

#define TRACE_INFO(name, size)                  \
    evaluation::TraceInfo {                     \
        #name,                                  \
        std::span<const int16_t>(s_trace.name), \
    },

/* create tracer and iterable to iterate the traces */
static inline TermTracer s_trace {};
constexpr auto s_traceIterable = std::to_array<TraceInfo>({ TERM_LIST(TRACE_INFO) });

}

#endif
