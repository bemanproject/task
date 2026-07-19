// include/beman/task/detail/into_optional.hpp                        -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_BEMAN_TASK_DETAIL_INTO_OPTIONAL
#define INCLUDED_BEMAN_TASK_DETAIL_INTO_OPTIONAL

#include <optional>
#include <tuple>
#include <beman/execution/execution.hpp>

// ----------------------------------------------------------------------------

namespace beman::task::detail {

inline constexpr struct into_optional_t : beman::execution::sender_adaptor_closure<into_optional_t> {
    template <::beman::execution::sender Upstream>
    struct sender {
        using sender_concept = ::beman::execution::sender_tag;
        using upstream_t     = std::remove_cvref_t<Upstream>;
        upstream_t upstream;

        template <typename...>
        struct type_list {};

        template <typename T>
        static auto find_type(type_list<type_list<T>>) {
            return std::optional<T>{};
        }
        template <typename T>
        static auto find_type(type_list<type_list<T>, type_list<>>) {
            return std::optional<T>{};
        }
        template <typename T>
        static auto find_type(type_list<type_list<>, type_list<T>>) {
            return std::optional<T>{};
        }
        template <typename T0, typename T1, typename... T>
        static auto find_type(type_list<type_list<T0, T1, T...>>) {
            return std::optional<std::tuple<T0, T1, T...>>{};
        }
        template <typename T0, typename T1, typename... T>
        static auto find_type(type_list<type_list<T0, T1, T...>, type_list<>>) {
            return std::optional<std::tuple<T0, T1, T...>>{};
        }
        template <typename T0, typename T1, typename... T>
        static auto find_type(type_list<type_list<>, type_list<T0, T1, T...>>) {
            return std::optional<std::tuple<T0, T1, T...>>{};
        }

        template <typename Env>
        static consteval auto get_type() {
            return find_type(
                ::beman::execution::value_types_of_t<Upstream, std::remove_cvref_t<Env>, type_list, type_list>());
        }

        template <typename Env, typename... E, typename... S>
        static consteval auto make_signatures(type_list<E...>, type_list<S...>) {
            return ::beman::execution::completion_signatures<::beman::execution::set_value_t(
                                                                 decltype(get_type<Env>())),
                                                             ::beman::execution::set_error_t(E)...,
                                                             S...>();
        }
        template <typename, typename Env>
        static consteval auto get_completion_signatures() {
            static_assert(::beman::execution::sender<Upstream>);
            static_assert(::beman::execution::sender_in<Upstream>);
            static_assert(::beman::execution::sender_in<Upstream, Env>);
            return make_signatures<Env>(
                ::beman::execution::value_types_of_t<Upstream, std::remove_cvref_t<Env>, type_list>{},
                ::beman::execution::error_types_of_t<Upstream, std::remove_cvref_t<Env>, type_list>{},
                std::conditional_t<::beman::execution::sends_stopped<Upstream, std::remove_cvref_t<Env>>,
                                   type_list<::beman::execution::set_stopped_t()>,
                                   type_list<>>{});
        }
        template <typename Receiver>
        struct make_object {
            template <typename... A>
            auto operator()(A&&... a) const
                -> decltype(get_type<decltype(::beman::execution::get_env(std::declval<Receiver>()))>()) {
                if constexpr (sizeof...(A) == 0u)
                    return {};
                else
                    return {std::forward<A>(a)...};
            }
        };
        template <typename Receiver>
        auto connect(Receiver&& receiver) && {
            return ::beman::execution::connect(
                ::beman::execution::then(std::move(this->upstream), make_object<Receiver>{}),
                std::forward<Receiver>(receiver));
        }
    };

    template <typename Upstream>
    sender<Upstream> operator()(Upstream&& upstream) const {
        return {std::forward<Upstream>(upstream)};
    }
} into_optional{};
} // namespace beman::task::detail

// ----------------------------------------------------------------------------

#endif
