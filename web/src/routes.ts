import type { Component } from "vue";
import type { RouteRecordRaw } from "vue-router";

export interface AppRouteComponents {
    debug: Component;
    dps: Component;
}

export function createAppRoutes(
    components: AppRouteComponents,
): RouteRecordRaw[] {
    return [
        {
            path: "/",
            name: "debug",
            component: components.debug,
        },
        {
            path: "/debug",
            redirect: "/",
        },
        {
            path: "/dps",
            name: "dps",
            component: components.dps,
        },
    ];
}
