#ifndef PIPE_H_INC
#define PIPE_H_INC

#include "metaprogramming.h"

namespace metaprog {

template <class Arg, class ArgCall, class OuterCall>
class pipe {
private:
    ArgCall argcall;
    OuterCall outercall;
public:
	typedef pipe<Arg, ArgCall, OuterCall>  this_type;
	pipe(ArgCall ac, OuterCall oc) : argcall(ac), outercall(oc) {}
	auto operator()(Arg arg) -> decltype(outercall(argcall(arg))) {
		return outercall(argcall(arg));
	}
	template <class NewCall>
	pipe<Arg, this_type, NewCall> operator[](NewCall&& nc) {
		return {*this, forward<NewCall>(nc)};
	}
};

template <class Arg>
class pipe_source {
public:
	typedef pipe_source<Arg> this_type;
	Arg operator()(Arg arg) {
		return arg;
	}
	template <class ArgCall, class OuterCall>
	static pipe<Arg, ArgCall, OuterCall> create(ArgCall&& ac, OuterCall&& oc) {
		return {forward<ArgCall>(ac), forward<OuterCall>(oc)};
	}
	template <class OuterCall>
	pipe<Arg, this_type, OuterCall> operator[](OuterCall&& oc) {
		return {*this, forward<OuterCall>(oc)};
	}
};

}

#endif
