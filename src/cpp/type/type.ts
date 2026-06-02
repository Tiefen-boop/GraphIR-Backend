
abstract class Type {
    abstract toString(): string;
}

class VoidType extends Type {
    toString(): string {
        return "void";
    }
}

class IntType extends Type {
    constructor(public size: 0 | 8 | 16 | 32 | 64 = 0) {
        super();
    }

    toString(): string {
        return this.size == 0 ? 'int' : `int${this.size}_t`;
    }
}

class UnsignedIntType extends Type {
    constructor(public size: 8 | 16 | 32 | 64) {
        super();
    }

    toString(): string {
        return `uint${this.size}_t`;
    }
}

class FloatType extends Type {
    constructor(public size: 32 | 64) {
        super();
    }

    toString(): string {
        return this.size == 32 ? "float" : "double";
    }
}

class BooleanType extends Type {
    toString(): string {
        return "bool";
    }
}

class StringType extends Type {
    toString(): string {
        return "std::string";
    }
}

class CharType extends Type {
    toString(): string {
        return "char";
    }
}

class TemplateType extends Type {
    constructor(public name: string, public parameters: Array<Type>) {
        super();
    }

    toString(): string {
        return `${this.name}<${this.parameters.map(p => p.toString()).join(", ")}>`;
    }
}

class ArrayType extends Type {
    constructor(public elementType: Type, public size: number) {
        super();
    }

    toString(): string {
        return `${this.elementType.toString()}[]`;
    }
}

class PointerType extends Type {
    constructor(public elementType: Type) {
        super();
    }

    toString(): string {
        return `${this.elementType.toString()}*`;
    }
}

class FunctionType extends Type {
    constructor(public returnType: Type, public parameters: Array<Type>) {
        super();
    }

    toString(): string {
        return `std::function<${this.returnType.toString()}(${this.parameters.map(p => p.toString()).join(", ")})>`;
    }
}

class AutoType extends Type {
    toString(): string {
        return "auto";
    }
}

class RefereceType extends Type {
    constructor(public elementType: Type) {
        super();
    }

    toString(): string {
        return `${this.elementType.toString()}&`;
    }
}

class ConstRefType extends Type {
    constructor(public elementType: Type) {
        super();
    }

    toString(): string {
        return `const ${this.elementType.toString()}&`;
    }
}

class ConstPointerType extends Type {
    constructor(public elementType: Type) {
        super();
    }

    toString(): string {
        return `const ${this.elementType.toString()}*`;
    }
}

class ScopedType extends Type {
    constructor(public scope: string, public name: string) {
        super();
    }

    toString(): string {
        return `${this.scope}::${this.name}`;
    }
}

export { Type, ArrayType, TemplateType, PointerType, IntType, UnsignedIntType, FloatType, BooleanType, StringType, CharType, VoidType, FunctionType, AutoType, RefereceType, ConstRefType, ConstPointerType, ScopedType };
