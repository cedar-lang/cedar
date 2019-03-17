/*
 * MIT License
 *
 * Copyright (c) 2018 Nick Wanninger
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <cedar/globals.h>
#include <cedar/modules.h>
#include <cedar/object/dict.h>
#include <cedar/object/fiber.h>
#include <cedar/object/keyword.h>
#include <cedar/object/module.h>
#include <cedar/object/string.h>
#include <cedar/object/vector.h>
#include <cedar/objtype.h>
#include <cedar/scheduler.h>
#include <cedar/vm/binding.h>
#include <uv.h>


using namespace cedar;


static type *tcp_type = nullptr;


class server_obj : public object {
 public:
  uv_tcp_t m_tcp;
  lambda *m_cb;
  inline server_obj(lambda *cb) {
    m_cb = cb;
    m_type = tcp_type;
  }
};


UV_EXTERN int uv_ip4_addr(const char *ip, int port, struct sockaddr_in *addr);

void bind_tcp(void) {
  tcp_type = new type("Server");


  tcp_type->setattr("__alloc__", bind_lambda(argc, argv, machine) {
    //
    return new server_obj(nullptr);
  });


  tcp_type->set_field("new", bind_lambda(argc, argv, ctx) {
    /*
  if (argc != 2) {
    throw cedar::make_exception("tcp.Server.new requires 1 argument");
  }
  if (argv[1].get_type() != lambda_type) {
    throw cedar::make_exception("tcp.Server.new requires a lambda callback");
  }
  lambda *cb = ref_cast<lambda>(argv[1]);
  server_obj *server = ref_cast<server_obj>(argv[0]);
  server->m_cb = cb;
  uv_tcp_init(ctx->schd->loop, &server->m_tcp);
  */
    return nullptr;
  });


  tcp_type->set_field("listen", bind_lambda(argc, argv, ctx) {
    if (argc != 3) {
      throw cedar::make_exception("tcp.Server.listen requires 2 arguments");
    }

    server_obj *self = ref_cast<server_obj>(argv[0]);


    if (argv[1].get_type() != string_type) {
      throw cedar::make_exception(
          "tcp.Server.listen requires the form (String, Number)");
    }
    if (argv[2].get_type() != number_type) {
      throw cedar::make_exception(
          "tcp.Server.listen requires the form (String, Number)");
    }

    sockaddr_in *addr = new sockaddr_in();
    std::string path = argv[1].to_string(true);
    const char *ip = path.c_str();
    int port = argv[2].to_int();

    int addr_res = uv_ip4_addr(ip, port, addr);

    self->m_tcp.data = self;



    return addr_res;
  });



  module *mod = new module("tcp");

  mod->def("Server", tcp_type);

  define_builtin_module("tcp", mod);
}
