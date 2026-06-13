import type {
    DevelopmentPlayerChaseScenario,
    EngineModule,
} from "./wasm/EngineModule";
export { loadEngineModule } from "./wasm/EngineModule";

export interface RegisteredScenario {
    name: "Player Chase";
    create(module: EngineModule): DevelopmentPlayerChaseScenario;
}

export const registeredScenarios: RegisteredScenario[] = [
    {
        name: "Player Chase",
        create(module) {
            return new module.DevelopmentPlayerChaseScenario();
        },
    },
];

export function createPlayerChaseScenario(
    module: EngineModule,
): DevelopmentPlayerChaseScenario {
    return registeredScenarios[0].create(module);
}
