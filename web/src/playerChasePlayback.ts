import type { PlayerChaseScenario } from "./scenarios";

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
    reset(): void;
    stop(): void;
}

export function createPlayerChasePlayback(
    scenario: PlayerChaseScenario,
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
        scenario.step();
        refreshSnapshot();
    }

    return {
        resume(): void {
            scenario.setRunning(true);
            refreshSnapshot();

            if (timerId === undefined) {
                timerId = scheduler.setInterval(tick, playbackTickIntervalMs);
            }
        },

        pause(): void {
            scenario.setRunning(false);
            clearTimer();
            refreshSnapshot();
        },

        toggle(): void {
            if (scenario.isRunning()) {
                this.pause();
                return;
            }

            this.resume();
        },

        stepWhilePaused(): boolean {
            if (scenario.isRunning()) {
                return false;
            }

            tick();
            return true;
        },

        reset(): void {
            clearTimer();
            scenario.reset();
            refreshSnapshot();
        },

        stop(): void {
            clearTimer();
        },
    };
}
