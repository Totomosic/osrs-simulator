<template>
  <main class="viewer-shell">
    <section class="status-panel" aria-labelledby="viewer-title">
      <p class="eyebrow">Development viewer</p>
      <h1 id="viewer-title">OSRS Simulator</h1>
      <dl>
        <div>
          <dt>Web app</dt>
          <dd>Ready</dd>
        </div>
        <div>
          <dt>Engine module</dt>
          <dd>Pending wasm build</dd>
        </div>
      </dl>
    </section>
  </main>
</template>

<style scoped>
.viewer-shell {
    align-items: center;
    background: #f5f7fb;
    color: #1f2937;
    display: flex;
    font-family:
        Inter, ui-sans-serif, system-ui, -apple-system, BlinkMacSystemFont,
        "Segoe UI", sans-serif;
    min-height: 100vh;
    padding: 32px;
}

.status-panel {
    border-left: 4px solid #2563eb;
    max-width: 560px;
    padding-left: 24px;
}

.eyebrow {
    color: #4b5563;
    font-size: 0.875rem;
    font-weight: 700;
    letter-spacing: 0;
    margin: 0 0 8px;
    text-transform: uppercase;
}

h1 {
    font-size: 2.5rem;
    line-height: 1.1;
    margin: 0 0 24px;
}

dl {
    display: grid;
    gap: 12px;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    margin: 0;
}

dt {
    color: #4b5563;
    font-size: 0.875rem;
    font-weight: 700;
    margin-bottom: 4px;
}

dd {
    font-size: 1rem;
    margin: 0;
}

@media (max-width: 520px) {
    .viewer-shell {
        align-items: flex-start;
        padding: 24px;
    }

    dl {
        grid-template-columns: 1fr;
    }
}
</style>
