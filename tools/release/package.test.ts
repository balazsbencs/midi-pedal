import { mkdtemp, mkdir, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { describe, expect, it } from "vitest";

import { packageRelease } from "./package.mjs";

describe("release packager", () => {
  it("refuses a package missing any public artifact", async () => {
    const root = await mkdtemp(join(tmpdir(), "midi-pedal-release-"));
    await mkdir(join(root, "editor/dist"), { recursive: true });
    await writeFile(join(root, "editor/dist/index.html"), "ok");
    await expect(packageRelease({ root, outDir: join(root, "out"), version: "0.1.0" })).rejects.toThrow("firmware.uf2");
  });

  it("reports the editor archive as a required public artifact", async () => {
    const root = await mkdtemp(join(tmpdir(), "midi-pedal-release-"));
    await expect(packageRelease({ root, outDir: join(root, "out"), version: "0.1.0" })).rejects.toThrow("editor-dist.zip");
  });
});

