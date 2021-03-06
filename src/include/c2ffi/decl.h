/*  -*- c++ -*-

    c2ffi
    Copyright (C) 2013  Ryan Pavlik

    This file is part of c2ffi.

    c2ffi is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    c2ffi is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with c2ffi.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef C2FFI_DECL_H
#define C2FFI_DECL_H

#include <string>
#include <utility>
#include <vector>
#include <clang/AST/Decl.h>
#include "c2ffi.h"
#include "c2ffi/type.h"

namespace c2ffi {
    typedef std::string Name;
    typedef std::vector<Name> NameVector;

    typedef std::pair<Name, Type*> NameTypePair;
    typedef std::vector<NameTypePair> NameTypeVector;

    typedef std::pair<Name, uint64_t> NameNumPair;
    typedef std::vector<NameNumPair> NameNumVector;

    class Decl : public Writable {
        std::string _name;
        std::string _loc;
        unsigned int _id;

    public:
        Decl(std::string name)
            : _name(name), _id(0) { }
        Decl(clang::NamedDecl *d);
        virtual ~Decl() { }

        virtual const std::string& name() const { return _name; }
        virtual const std::string& location() const { return _loc; }

        unsigned int id() const { return _id; }
        void set_id(unsigned int id) { _id = id; }

        virtual void set_location(const std::string &loc) { _loc = loc; }
        virtual void set_location(clang::CompilerInstance &ci, const clang::Decl *d);
    };

    class UnhandledDecl : public Decl {
        std::string _kind;
    public:
        UnhandledDecl(std::string name, std::string kind)
            : Decl(name), _kind(kind) { }

        virtual void write(OutputDriver &od) const { od.write((const UnhandledDecl&)*this); }
        const std::string& kind() const { return _kind; }
    };

    class TypeDecl : public Decl {
        Type *_type;
    public:
        TypeDecl(std::string name, Type *type)
            : Decl(name), _type(type) { }
        virtual ~TypeDecl() { delete _type; }

        virtual void write(OutputDriver &od) const { od.write((const TypeDecl&)*this); }
        virtual const Type& type() const { return *_type; }
    };

    class VarDecl : public TypeDecl {
        std::string _value;
        bool _is_extern;
        bool _is_string;

    public:
        VarDecl(std::string name, Type *type, std::string value = "",
                bool is_extern = false, bool is_string = false)
            : TypeDecl(name, type), _value(value), _is_extern(is_extern),
              _is_string(is_string) { }

        virtual void write(OutputDriver &od) const { od.write((const VarDecl&)*this); }

        const std::string& value() const { return _value; }
        bool is_extern() const { return _is_extern; }

        bool is_string() const { return _is_string; }
        bool set_is_string(bool v) { return _is_string = v; }
    };

    class FieldsMixin {
        NameTypeVector _v;
    public:
        virtual ~FieldsMixin();

        void add_field(Name, Type*);
        void add_field(C2FFIASTConsumer *ast, clang::FieldDecl *f);
        void add_field(C2FFIASTConsumer *ast, clang::ParmVarDecl *v);
        const NameTypeVector& fields() const { return _v; }
    };

    class FunctionDecl : public Decl, public FieldsMixin {
        Type *_return;
        bool _is_variadic;
        bool _is_objc_method;
        bool _is_class_method;
    public:
        FunctionDecl(std::string name, Type *type, bool is_variadic)
            : Decl(name), _return(type), _is_variadic(is_variadic),
              _is_class_method(false), _is_objc_method(false) { }

        virtual void write(OutputDriver &od) const { od.write((const FunctionDecl&)*this); }

        virtual const Type& return_type() const { return *_return; }

        bool is_variadic() const { return _is_variadic; }
        bool is_objc_method() const { return _is_objc_method; }
        void set_is_objc_method(bool val) {
            _is_objc_method = val;
        }

        bool is_class_method() const { return _is_class_method; }
        void set_is_class_method(bool val) {
            _is_class_method = val;
        }
    };

    typedef std::vector<FunctionDecl*> FunctionVector;

    class FunctionsMixin {
        FunctionVector _v;
    public:
        virtual ~FunctionsMixin();

        void add_function(FunctionDecl *f);
        const FunctionVector& functions() const { return _v; }

        void add_functions(C2FFIASTConsumer *ast, const clang::ObjCContainerDecl *d);
    };

    class TypedefDecl : public TypeDecl {
    public:
        TypedefDecl(std::string name, Type *type)
            : TypeDecl(name, type) { }

        virtual void write(OutputDriver &od) const { od.write((const TypedefDecl&)*this); }
    };

    class RecordDecl : public Decl, public FieldsMixin {
        bool _is_union;
        NameTypeVector _v;

        uint64_t _bit_size;
        unsigned _bit_alignment;

    public:
        RecordDecl(std::string name, bool is_union = false)
            : Decl(name), _is_union(is_union), _bit_size(0),
              _bit_alignment(0)
        { }

        virtual void write(OutputDriver &od) const { od.write((const RecordDecl&)*this); }
        bool is_union() const { return _is_union; }

        uint64_t bit_size() const { return _bit_size; }
        void set_bit_size(uint64_t size) { _bit_size = size; }

        uint64_t bit_alignment() const { return _bit_alignment; }
        void set_bit_alignment(uint64_t alignment) { _bit_alignment = alignment; }
    };

    class EnumDecl : public Decl {
        NameNumVector _v;
    public:
        EnumDecl(std::string name) : Decl(name) { }

        virtual void write(OutputDriver &od) const { od.write((const EnumDecl&)*this); }

        void add_field(std::string, uint64_t);
        const NameNumVector& fields() const { return _v; }
    };

    /** ObjC **/
    class ObjCInterfaceDecl : public Decl, public FieldsMixin,
                              public FunctionsMixin {
        std::string _super;
        bool _is_forward;
        NameVector _protocols;
    public:
        ObjCInterfaceDecl(std::string name, std::string super,
                          bool is_forward)
            : Decl(name), _super(super), _is_forward(is_forward) { }

        virtual void write(OutputDriver &od) const { od.write((const ObjCInterfaceDecl&)*this); }

        const std::string& super() const { return _super; }
        bool is_forward() const { return _is_forward; }

        void add_protocol(Name proto);
        const NameVector& protocols() const { return _protocols; }
    };

    class ObjCCategoryDecl : public Decl, public FunctionsMixin {
        Name _category;

    public:
        ObjCCategoryDecl(Name name, Name category)
            : Decl(name), _category(category) { }

        virtual void write(OutputDriver &od) const { od.write((const ObjCCategoryDecl&)*this); }

        const Name& category() const { return _category; }
    };

    class ObjCProtocolDecl : public Decl, public FunctionsMixin {
    public:
        ObjCProtocolDecl(Name name)
            : Decl(name) { }

        virtual void write(OutputDriver &od) const { od.write((const ObjCProtocolDecl&)*this); }
    };
}

#endif /* C2FFI_DECL_H */
