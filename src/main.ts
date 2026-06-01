
import assert from 'assert';
import * as ir from 'graphir'

import { CodeGenIterable } from './codegen_iterable.js';
import { irTypeToCppType } from './cpp/type/type_conversion.js';

import * as cppType from './cpp/type/type.js';
import * as decl from './cpp/ast/decl.js';
import * as stmt from './cpp/ast/stmt.js';
import * as expr from './cpp/ast/expr.js';
import { CppCodeGenVisitor, AstNode } from './cpp/code_gen.js';
import { allocateCppNames } from './cpp/names_allocator.js';

import { BackendConfig } from './config.js';

export function generateCpp(graph: ir.Graph, config?: BackendConfig): string {
    if (!config) {
        config = new BackendConfig();
    }

    let out = '';

    for (const subgraph of graph.subgraphs) {
        const subfunction_type = (irTypeToCppType(subgraph.verifiedType!) as cppType.FunctionType);
        const subfunction_name = (subgraph.getStartVertex().inEdges[0].source as ir.StaticSymbolVertex).name;

        const paramTypes = subfunction_type.parameters.map((t, i) =>
            config!.constRefParams.get(subfunction_name)?.has(i) ? new cppType.ConstRefType(t) : t
        );
        const subfunction_declaration = new decl.FuncDecl(subfunction_type.returnType, subfunction_name, paramTypes);
        out += subfunction_declaration.toString() + '\n';
    }

    out += '\n';

    for (const subgraph of graph.subgraphs) {
        out += generateCpp(subgraph, config);
    }
    let function_name;
    if (graph.getStartVertex().inEdges.length > 0) {
        function_name = (graph.getStartVertex().inEdges[0].source as ir.StaticSymbolVertex).name;
    }
    else {
        function_name = 'main';
    }
    const function_type = (irTypeToCppType(graph.verifiedType!) as cppType.FunctionType);
    const parameters = function_type.parameters.map((t, i) => {
        const type = config!.constRefParams.get(function_name)?.has(i) ? new cppType.ConstRefType(t) : t;
        return new decl.ParamDecl(type, `p${i}`);
    });
    const cpp_function = new decl.FuncDefDecl(function_type.returnType, function_name, parameters, new stmt.BlockStmt([]));
    const names = allocateCppNames(graph);

    const dataVertices = [...new CodeGenIterable(graph)]
        .filter(v => (v instanceof ir.DataVertex || v instanceof ir.CompoundVertex) && !(v instanceof ir.StaticSymbolVertex))
        .filter(v => !(v instanceof ir.ParameterVertex))

    const variableDeclarations = dataVertices
        .filter(v => !((v as ir.DataVertex).verifiedType! instanceof ir.VoidType) && !((v as ir.DataVertex).verifiedType! instanceof ir.FunctionType))
        .map(v => {
            let type = irTypeToCppType((v as ir.DataVertex).verifiedType!);
            if (v instanceof ir.LoadVertex && (v.verifiedType instanceof ir.DynamicArrayType || (v.verifiedType instanceof ir.UnionType && v.verifiedType.types.some(t => t instanceof ir.DynamicArrayType)))) {
                type = new cppType.PointerType(type);
            }
            return new decl.VarDecl(type, names.get(v)!)
        });
    cpp_function.body.statements.push(...variableDeclarations);

    const functorVariableDeclarations = dataVertices
        .filter(v => (v as ir.DataVertex).verifiedType! instanceof ir.FunctionType)
        .map(v => {
            assert(v instanceof ir.LoadVertex);
            let type;
            if (v.object instanceof ir.StaticSymbolVertex && v.property instanceof ir.StaticSymbolVertex) {
                type = new cppType.ScopedType(v.object.name, v.property.name);
            }
            else {
                type = irTypeToCppType(v.verifiedType!);
            }
            return new decl.VarDecl(type, names.get(v)!);
        });
    cpp_function.body.statements.push(...functorVariableDeclarations);

    const instructionGenVisitor = new CppCodeGenVisitor(names);
    const iterableGraph = new CodeGenIterable(graph);
    for (let vertex of iterableGraph) {
        const statement = vertex.accept(instructionGenVisitor);
        if (!statement) {
            continue;
        }
        cpp_function.body.statements.push(...statement);
    }
    if (function_name === 'main') {
        cpp_function.parameters = [
            new decl.ParamDecl(new cppType.IntType(), 'argc'),
            new decl.ParamDecl(new cppType.PointerType(new cppType.PointerType(new cppType.CharType())), 'argv')
        ];
        cpp_function.body.statements.unshift(
            new stmt.ExprStmt(new expr.CallExpr('process::initializeArgv', [
                new expr.IdentifierExpr('argc'),
                new expr.IdentifierExpr('argv')
            ])),
        );
        cpp_function.body.statements.pop();
        cpp_function.body.statements.push(new stmt.ReturnStmt(new expr.LiteralExpr(0)));
    }

    out += cpp_function.toString() + '\n';
    return out;
}
