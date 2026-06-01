
export class BackendConfig {
    public assumptions = {
        noLocalOwnership: false,
    }
    // Maps function name → set of param indices to emit as `const T&` (non-mutating array params).
    public constRefParams: Map<string, Set<number>> = new Map();
}
