import type { DevelopmentPlayerChaseScenario } from "./wasm/EngineModule";

export const playbackTickIntervalMs = 600;

export interface PlaybackScheduler {
    setInterval(callback: () => void, delayMs: number): number;
    clearInterval(timerId: number): void;
}

export interface PlayerChasePlaybackControls {
    resume(): void;
    pause(): void;
    toggle(): void;
    stepWhilePaused(): boolean;
    stop(): void;
}

export function createPlayerChasePlayback(
    scenario: DevelopmentPlayerChaseScenario,
    refreshSnapshot: () => void,
    scheduler: PlaybackScheduler = window,
): PlayerChasePlaybackControls {
    let timerId: number | undefined;

    function clearTimer(): void {
        if (timerId !== undefined) {
            scheduler.clearInterval(timerId);
            timerId = undefined;
        }
    }

    function tick(): void {
        scenario.Step();
        refreshSnapshot();
    }

    return {
        resume(): void {
            scenario.SetRunning(true);
            refreshSnapshot();

            if (timerId === undefined) {
                timerId = scheduler.setInterval(tick, playbackTickIntervalMs);
            }
        },

        pause(): void {
            scenario.SetRunning(false);
            clearTimer();
            refreshSnapshot();
        },

        toggle(): void {
            if (scenario.IsRunning()) {
                this.pause();
                return;
            }

            this.resume();
        },

        stepWhilePaused(): boolean {
            if (scenario.IsRunning()) {
                return false;
            }

            tick();
            return true;
        },

        stop(): void {
            clearTimer();
        },
    };
}
