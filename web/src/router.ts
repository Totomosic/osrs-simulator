import { createRouter, createWebHistory } from "vue-router";
import DpsCalculatorView from "./DpsCalculatorView.vue";
import PlayerChaseView from "./PlayerChaseView.vue";
import { createAppRoutes } from "./routes";

export const router = createRouter({
    history: createWebHistory(),
    routes: createAppRoutes({
        debug: PlayerChaseView,
        dps: DpsCalculatorView,
    }),
});
