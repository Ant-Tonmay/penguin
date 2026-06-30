#include "parser/parser.h"
#include "lexer/lexer.h"
#include <iostream>
#include <vector>
#include <cassert>
#include <string>

// Helper macros for verification
#define ASSERT_NOT_NULL(ptr) if (ptr == nullptr) { std::cerr << "Assertion failed: " #ptr " is null at line " << __LINE__ << std::endl; exit(1); }
#define ASSERT_EQ(val1, val2) if (val1 != val2) { std::cerr << "Assertion failed: " #val1 " (" << val1 << ") != " #val2 " (" << val2 << ") at line " << __LINE__ << std::endl; exit(1); }

std::unique_ptr<Program> parseSource(const std::string& source) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    return parser.parse();
}

void testEmptyClass() {
    std::cout << "Testing Empty Class..." << std::endl;
    std::string source = "{\n"
                         "    class Empty {}\n"
                         "}";
    auto program = parseSource(source);
    ASSERT_NOT_NULL(program);
    ASSERT_EQ(program->classes.size(), 1);
    
    auto& cls = program->classes[0];
    ASSERT_EQ(cls->name, "Empty");
    ASSERT_EQ(cls->sections.size(), 0);
}

void testClassWithFields() {
    std::cout << "Testing Class with Fields..." << std::endl;
    std::string source = "{\n"
                         "    class Data {\n"
                         "        dec: x;\n"
                         "        dec: y;\n"
                         "    }\n"
                         "}";

    auto program = parseSource(source);
    ASSERT_NOT_NULL(program);
    ASSERT_EQ(program->classes.size(), 1);
    
    auto& cls = program->classes[0];
    ASSERT_EQ(cls->name, "Data");
    
    // Direct members are wrapped in a PRIVATE section
    // Here we have 2 fields, so 2 wrapper sections (one for each loop iteration in parser)
    ASSERT_EQ(cls->sections.size(), 2); 
    
    ASSERT_EQ(cls->sections[0]->members.size(), 1);
    auto* field1 = dynamic_cast<FieldDecl*>(cls->sections[0]->members[0].get());
    ASSERT_NOT_NULL(field1);
    ASSERT_EQ(field1->name, "x");

    ASSERT_EQ(cls->sections[1]->members.size(), 1);
    auto* field2 = dynamic_cast<FieldDecl*>(cls->sections[1]->members[0].get());
    ASSERT_NOT_NULL(field2);
    ASSERT_EQ(field2->name, "y");
}

void testClassWithMethods() {
    std::cout << "Testing Class with Methods..." << std::endl;
    std::string source = "{\n"
                         "    class Calculator {\n"
                         "        func add(a, b) { return a + b; }\n"
                         "    }\n"
                         "}";

    auto program = parseSource(source);
    ASSERT_NOT_NULL(program);
    auto& cls = program->classes[0];
    
    ASSERT_EQ(cls->sections.size(), 1);
    auto& sec = cls->sections[0];
    ASSERT_EQ(sec->members.size(), 1);
    
    auto* method = dynamic_cast<MethodDef*>(sec->members[0].get());
    ASSERT_NOT_NULL(method);
    ASSERT_EQ(method->name, "add");
    ASSERT_EQ(method->params.size(), 2);
    ASSERT_EQ(method->params[0].name, "a");
    ASSERT_EQ(method->params[1].name, "b");
}

void testAccessModifiers() {
    std::cout << "Testing Access Modifiers..." << std::endl;
    std::string source = "{\n"
                         "    class Access {\n"
                         "        public { dec: pub; }\n"
                         "        private { dec: priv; }\n"
                         "    }\n"
                         "}";

    auto program = parseSource(source);
    ASSERT_NOT_NULL(program);
    auto& cls = program->classes[0];
    
    ASSERT_EQ(cls->sections.size(), 2);
    
    // First section: public
    auto& sec1 = cls->sections[0];
    ASSERT_EQ((int)sec1->modifier, (int)AccessModifier::PUBLIC);
    ASSERT_EQ(sec1->members.size(), 1);
    auto* field1 = dynamic_cast<FieldDecl*>(sec1->members[0].get());
    ASSERT_EQ(field1->name, "pub");
    
    // Second section: private
    auto& sec2 = cls->sections[1];
    ASSERT_EQ((int)sec2->modifier, (int)AccessModifier::PRIVATE);
    ASSERT_EQ(sec2->members.size(), 1);
    auto* field2 = dynamic_cast<FieldDecl*>(sec2->members[0].get());
    ASSERT_EQ(field2->name, "priv");
}

void testHashMapClass() {
    std::cout << "Testing HashMap Class (Example)..." << std::endl;
    std::string source = "{\n"
                         "    class HashMap {\n"
                         "       private {\n"
                         "            dec : arr;\n"
                         "            dec : insert(key,val);\n"
                         "            dec : retreive(key);\n"
                         "            dec : change(key , val);\n"
                         "       }\n"
                         "       public{\n"
                         "            dec : push(key,val);\n"
                         "            dec : get(key);\n"
                         "            dec : update(key , val);\n"
                         "\n"
                         "            func HashMap(){\n"
                         "                this.arr = [];\n"
                         "            }\n"
                         "\n"
                         "            func printMap(){\n"
                         "            }\n"
                         "       }\n"
                         "\n"
                         "       func insert(key,val){\n"
                         "       }\n"
                         "       func retreive(key){\n"
                         "       }\n"
                         "       func change(key){\n"
                         "       }\n"
                         "\n"
                         "       func push(key , val){\n"
                         "            insert(key,val);\n"
                         "       }\n"
                         "       func get(key){\n"
                         "            return retreive(key);\n"
                         "       }\n"
                         "       func update(key ,val){\n"
                         "            change(key,val);\n"
                         "       }\n"
                         "    }\n"
                         "}";

    auto program = parseSource(source);
    ASSERT_NOT_NULL(program);
    ASSERT_EQ(program->classes.size(), 1);
    auto& cls = program->classes[0];
    ASSERT_EQ(cls->name, "HashMap");

    // Sections: private, public, then implicit private sections for each direct method
    // private block, public block, insert, retreive, change, push, get, update -> 8 sections total
    // Let's verify content roughly
    
    // 1. Private block
    auto& sec1 = cls->sections[0];
    ASSERT_EQ((int)sec1->modifier, (int)AccessModifier::PRIVATE);
    ASSERT_EQ(sec1->members.size(), 4); // arr, insert, retreive, change (decls)
    
    // 2. Public block
    auto& sec2 = cls->sections[1];
    ASSERT_EQ((int)sec2->modifier, (int)AccessModifier::PUBLIC);
    ASSERT_EQ(sec2->members.size(), 5); // push, get, update (decls), HashMap, printMap (defs)

    // 3. insert method
    auto& sec3 = cls->sections[2];
    auto* method1 = dynamic_cast<MethodDef*>(sec3->members[0].get());
    ASSERT_NOT_NULL(method1);
    ASSERT_EQ(method1->name, "insert");
}

int main() {
    std::cout << "Running Class Parser Tests..." << std::endl;
    testEmptyClass();
    testClassWithFields();
    testClassWithMethods();
    testAccessModifiers();
    testHashMapClass();
    std::cout << "All class parser tests passed!" << std::endl;
    return 0;
}
