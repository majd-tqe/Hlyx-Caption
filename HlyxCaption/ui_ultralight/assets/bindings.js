// Hlyx Caption original UI source — SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Hlyx Caption Contributors
// bindings.js - UL settings panel
// Stores all current values in a single global object (window.__state).
// On Apply, native polls this object via EvaluateScript("__getConfigState()").
// On Reset, native refreshes DOM via EvaluateScript calls.

(function () {
    "use strict";

    // The panel has two explicit languages.  ui_language=1 is English and
    // ui_language=2 is Arabic; old/invalid values fall back to Arabic.
    const TRANSLATIONS = {
        ar: {
            "page.title": "Hlyx Caption — إعدادات الترجمة",
            "brand.title": "Hlyx Caption",
            "brand.badge": "الإعدادات",
            "hints.settings": "قائمة الإعدادات",
            "translation.enabled": "الترجمة مفعلة",
            "translation.disabled": "الترجمة موقوفة",
            "actions.close": "إغلاق",
            "actions.preview": "معاينة",
            "actions.save": "حفظ",
            "actions.reset": "استعادة الافتراضي",
            "stats.font_size": "حجم الخط:",
            "stats.spacing": "تباعد:",
            "stats.reference": "مقياس:",
            "sections.typography": "الطباعة",
            "sections.alignment": "المحاذاة",
            "sections.text_color": "لون النص",
            "sections.outline": "الحدود (Outline)",
            "sections.shadow": "الظل",
            "sections.position": "موضع النصوص",
            "sections.timing": "التوقيت",
            "sections.font": "الخط",
            "sections.language": "اللغة",
            "labels.font_size": "حجم الخط",
            "labels.line_spacing": "تباعد الأسطر",
            "labels.reference_height": "الهامش الرئيسي",
            "labels.alignment": "المحاذاة",
            "labels.red": "أحمر (R)",
            "labels.green": "أخضر (G)",
            "labels.blue": "أزرق (B)",
            "labels.outline_enabled": "تفعيل الحدود",
            "labels.outline_thickness": "سمك الحدود",
            "labels.shadow_enabled": "إضافة ظل",
            "labels.shadow_offset_x": "سمك الظل (offset X)",
            "labels.position_x": "الموضع الأفقي (X)",
            "labels.position_y": "الموضع العمودي (Y)",
            "labels.fade_in": "الظهور (fade-in)",
            "labels.fade_out": "الاختفاء (fade-out)",
            "labels.extra_time": "الوقت الإضافي للجملة",
            "labels.custom_font": "الخط المخصص",
            "labels.fallback_font": "خط احتياطي",
            "labels.ui_language": "لغة الواجهة",
            "values.right": "يمين",
            "values.center": "وسط",
            "values.left": "يسار",
            "languages.english": "English",
            "languages.arabic": "العربية",
            "tabs.text": "النص",
            "tabs.color": "اللون",
            "tabs.outline": "الحدود",
            "tabs.shadow": "الظل",
            "tabs.position": "الموقع",
            "tabs.timing": "التوقيت",
            "tabs.font": "الخط",
            "tabs.general": "عام"
        },
        en: {
            "page.title": "Hlyx Caption — Caption Settings",
            "brand.title": "Hlyx Caption",
            "brand.badge": "SETTINGS",
            "hints.settings": "Settings",
            "translation.enabled": "Caption enabled",
            "translation.disabled": "Caption disabled",
            "actions.close": "Close",
            "actions.preview": "Preview",
            "actions.save": "Save",
            "actions.reset": "Restore Defaults",
            "stats.font_size": "Font size:",
            "stats.spacing": "Spacing:",
            "stats.reference": "Scale:",
            "sections.typography": "Typography",
            "sections.alignment": "Alignment",
            "sections.text_color": "Text Color",
            "sections.outline": "Outline",
            "sections.shadow": "Shadow",
            "sections.position": "Text Position",
            "sections.timing": "Timing",
            "sections.font": "Font",
            "sections.language": "Language",
            "labels.font_size": "Font size",
            "labels.line_spacing": "Line spacing",
            "labels.reference_height": "Reference height",
            "labels.alignment": "Alignment",
            "labels.red": "Red (R)",
            "labels.green": "Green (G)",
            "labels.blue": "Blue (B)",
            "labels.outline_enabled": "Enable outline",
            "labels.outline_thickness": "Outline thickness",
            "labels.shadow_enabled": "Enable shadow",
            "labels.shadow_offset_x": "Shadow offset X",
            "labels.position_x": "Horizontal position (X)",
            "labels.position_y": "Vertical position (Y)",
            "labels.fade_in": "Fade in",
            "labels.fade_out": "Fade out",
            "labels.extra_time": "Extra caption time",
            "labels.custom_font": "Custom font",
            "labels.fallback_font": "Fallback font",
            "labels.ui_language": "Interface language",
            "values.right": "Right",
            "values.center": "Center",
            "values.left": "Left",
            "languages.english": "English",
            "languages.arabic": "Arabic",
            "tabs.text": "Text",
            "tabs.color": "Color",
            "tabs.outline": "Outline",
            "tabs.shadow": "Shadow",
            "tabs.position": "Position",
            "tabs.timing": "Timing",
            "tabs.font": "Font",
            "tabs.general": "General"
        }
    };

    const PREVIEW_TEXTS = {
        ar: [
            "هذا نص تجريبي لمعاينة الترجمة — Preview 123",
            "سطر ثاني <clr:255,180,0>ملون<clr> و <B>عريض</B> و <I>مائل</I><cr>سطر ثالث للتجربة",
            "النصوص التجريبية تساعدك على معايرة الإعدادات بدقة"
        ],
        en: [
            "This is sample text for previewing captions — Preview 123",
            "A second line with <clr:255,180,0>color<clr>, <B>bold</B>, and <I>italics</I><cr>A third line for testing",
            "Preview text helps you fine-tune the settings"
        ]
    };

    let activeLanguage = "ar";

    function tr(key) {
        return (TRANSLATIONS[activeLanguage] && TRANSLATIONS[activeLanguage][key]) || key;
    }

    function getPreviewText() {
        return PREVIEW_TEXTS[activeLanguage].join("<cr>");
    }

    function applyLanguage(value) {
        const numeric = Number(value);
        activeLanguage = numeric === 1 ? "en" : "ar";
        const isEnglish = activeLanguage === "en";
        document.documentElement.lang = activeLanguage;
        document.documentElement.dir = isEnglish ? "ltr" : "rtl";
        document.body.classList.toggle("ui-ltr", isEnglish);
        document.body.classList.toggle("ui-rtl", !isEnglish);
        document.querySelectorAll("[data-i18n]").forEach((el) => {
            el.textContent = tr(el.getAttribute("data-i18n"));
        });
        const languageValue = Number(window.__state && window.__state.ui_language);
        const languageLabel = document.querySelector("#ui_language .fdrop-value");
        if (languageLabel) {
            languageLabel.textContent = tr(languageValue === 1
                ? "languages.english"
                : "languages.arabic");
        }
        window.__previewText = getPreviewText();
        if (window.__previewEnabled) window.__previewRequested = true;
        if (window.hlaBanner && window.__translationVisible !== undefined) {
            window.hlaBanner.setTranslationVisible(window.__translationVisible);
        }
    }

    // ============== Formatters ==============
    // Each entry maps a control id to a function (value -> display string).
    // NOTE: title-block stats reuse the same formatter so they stay in sync.
    const FORMATTERS = {
        extra_display_time:       v => v.toFixed(2) + "s",
        mouse_sensitivity:        v => v.toFixed(2) + "x",
        mouse_accel_factor:       v => v.toFixed(2) + "x",
        fade_in_time:             v => v.toFixed(2) + "s",
        fade_out_time:            v => v.toFixed(2) + "s",
        pos_x:                    v => Math.round(v * 100) + "%",
        pos_y:                    v => Math.round(v * 100) + "%",
        font_size:                v => v.toFixed(1) + " px",
        line_spacing:             v => v.toFixed(2),
        max_line_width_percent:   v => Math.round(v * 100) + "%",
        text_r:                   v => Math.round(v),
        text_g:                   v => Math.round(v),
        text_b:                   v => Math.round(v),
        shadow_offset_x:          v => v.toFixed(1) + " px",
        outline_thickness:        v => v.toFixed(1) + " px",
        bg_padding_x:             v => Math.round(v),
        bg_padding_y:             v => Math.round(v),
        bg_border_radius:         v => v.toFixed(1) + " px",
        font_size_reference_height: v => Math.round(v) + " px",
    };

    function fmt(id, v) {
        const f = FORMATTERS[id];
        return f ? f(v) : String(v);
    }

    // Paints the hazard-orange fill on a range input's track (the track
    // pseudo is transparent, so the input's own gradient shows through).
    // Color sliders keep their static R/G/B gradients.
    function paintSlider(input) {
        if (!input ||
            input.classList.contains("color-r") ||
            input.classList.contains("color-g") ||
            input.classList.contains("color-b")) return;
        const min = parseFloat(input.min), max = parseFloat(input.max), v = parseFloat(input.value);
        if (isNaN(min) || isNaN(max)) return;
        const pct = Math.max(0, Math.min(100, ((v - min) / (max - min)) * 100));
        input.style.background =
            "linear-gradient(90deg, var(--hazard) 0%, var(--hazard-bright) " +
            pct + "%, #262d2e " + pct + "%, #262d2e 100%)";
    }

    // ============== Tab switching ==============
    document.querySelectorAll(".nav-item").forEach((btn) => {
        btn.addEventListener("click", () => {
            const target = btn.dataset.tab;
            document.querySelectorAll(".nav-item").forEach((b) =>
                b.classList.toggle("active", b === btn));
            document.querySelectorAll(".content section").forEach((s) =>
                s.classList.toggle("active", s.id === target));
        });
    });

    // ============== Title-block stats sync ==============
    // Reflects a few "headline" values at the top of the panel so the user
    // sees the current configuration even before scrolling.
    // The stats use a tighter "compact" formatter than the inline slider values
    // so the title-bar reads as "34px" rather than "34.0 px".
    const STAT_IDS = {
        font_size:                "stat-font-size",
        line_spacing:             "stat-line-spacing",
        font_size_reference_height: "stat-reference-height",
    };
    const STAT_FORMATTERS = {
        font_size:                v => Math.round(v) + "px",
        line_spacing:             v => v.toFixed(2),
        font_size_reference_height: v => Math.round(v) + "px",
    };

    function refreshTitleStats() {
        for (const key in STAT_IDS) {
            const el = document.getElementById(STAT_IDS[key]);
            if (!el || !(key in window.__state)) continue;
            const f = STAT_FORMATTERS[key];
            el.textContent = f ? f(window.__state[key]) : String(window.__state[key]);
        }
    }

    // ============== Slider wiring ==============
    function wireSlider(id) {
        const input = document.getElementById(id);
        const out = document.getElementById(id + "_out");
        if (!input) return;
        const update = () => {
            const v = parseFloat(input.value);
            if (out) out.textContent = fmt(id, v);
            window.__state[id] = v;
            window.__stateDirty = true;
            paintSlider(input);
            refreshTitleStats();
        };
        input.addEventListener("input", update);
        update();
    }

    // ============== Checkbox / Switch wiring ==============
    function wireCheckbox(id) {
        const input = document.getElementById(id);
        if (!input) return;
        const update = () => { window.__state[id] = input.checked; window.__stateDirty = true; };
        input.addEventListener("change", update);
        update();
    }

    // ============== Radio (segmented control) wiring ==============
    function wireRadio(id) {
        const buttons = document.querySelectorAll('#' + id + ' .seg-btn');
        if (!buttons.length) return;
        buttons.forEach((btn) => {
            btn.addEventListener("click", () => {
                const v = parseInt(btn.dataset.value, 10);
                buttons.forEach((b) =>
                    b.classList.toggle("selected", b === btn));
                window.__state[id] = v;
                window.__stateDirty = true;
            });
        });
    }

    // ============== Text input wiring ==============
    function wireText(id) {
        const input = document.getElementById(id);
        if (!input) return;
        input.addEventListener("input", () => {
            window.__state[id] = input.value;
            window.__stateDirty = true;
        });
    }

// ============== Custom dropdown wiring (.fdrop) ==============
// The font pickers + the language picker are plain HTML button+list widgets
// (see settings.html) — no native <select> popup, so the FIRST click always
// opens the list (the WebKit popup of a native select needs view focus that
// the embedded view loses in-game, making the first click dead).
function dropSetValue(id, v) {
    const root = document.getElementById(id);
    if (!root) return;
    const label = root.querySelector(".fdrop-value");
    let found = null;
    root.querySelectorAll(".fdrop-item").forEach((li) => {
        const match = li.dataset.value === String(v);
        li.classList.toggle("selected", match);
        if (match) found = li;
    });
    if (found && label) label.textContent = found.textContent;
    else if (label) label.textContent = String(v);
}

// Position the dropdown list; with `position:absolute` the list is
// placed directly below the button via CSS `top:calc(100% + 4px)`.
// This helper only handles the flip-above case when there is not enough
// space below the button (e.g. near the bottom of the viewport) and
// ensures the list can escape the panel's scroll clipping.
function positionDrop(root) {
    const btn = root.querySelector(".fdrop-btn");
    const list = root.querySelector(".fdrop-list");
    if (!btn || !list) return;
    const r = btn.getBoundingClientRect();
    const vh = window.innerHeight || document.documentElement.clientHeight;
    const gap = 4;
    const listH = Math.min(220, list.scrollHeight + 8);
    // Reset to default (below)
    list.style.top = "calc(100% + 4px)";
    list.style.bottom = "auto";
    list.style.left = "0";
    list.style.right = "0";
    list.style.width = "";
    list.style.minWidth = "";
    list.style.maxHeight = "220px";
    // Flip above if not enough space below
    if (r.bottom + gap + listH > vh - 8) {
        list.style.top = "auto";
        list.style.bottom = "calc(100% + 4px)";
        list.style.maxHeight = Math.min(220, r.top - gap - 8) + "px";
    } else {
        list.style.maxHeight = Math.min(220, vh - r.bottom - gap - 8) + "px";
    }
}

function wireDrop(id, numeric) {
    const root = document.getElementById(id);
    if (!root) return;
    const btn = root.querySelector(".fdrop-btn");
    const list = root.querySelector(".fdrop-list");
    if (!btn || !list) return;
    const panelCol = document.querySelector(".panel-col");
    const appEl = document.getElementById("app");
    const close = () => {
        root.classList.remove("open");
        if (panelCol) panelCol.style.overflow = "";
        if (appEl) appEl.style.overflow = "";
        list.style.top = "";
        list.style.bottom = "";
        list.style.left = "";
        list.style.right = "";
        list.style.width = "";
        list.style.minWidth = "";
        list.style.maxHeight = "";
    };

    const openAndPosition = () => {
        document.querySelectorAll(".fdrop.open").forEach((el) => {
            if (el !== root) el.classList.remove("open");
        });
        root.classList.add("open");
        if (panelCol) panelCol.style.overflow = "visible";
        if (appEl) appEl.style.overflow = "visible";
        positionDrop(root);
    };

    btn.addEventListener("click", (e) => {
        e.stopPropagation();
        if (root.classList.contains("open")) {
            close();
        } else {
            openAndPosition();
        }
    });
    // Keyboard: Enter/Space toggles, Esc closes.
    btn.addEventListener("keydown", (e) => {
        if (e.key === "Enter" || e.key === " " || e.keyCode === 13 || e.keyCode === 32) {
            e.preventDefault();
            if (root.classList.contains("open")) close();
            else openAndPosition();
        } else if (e.key === "Escape" || e.keyCode === 27) {
            close();
        }
    });
    list.addEventListener("click", (e) => {
        const li = e.target.closest(".fdrop-item");
        if (!li) return;
        const v = li.dataset.value;
        dropSetValue(id, v);
        window.__state[id] = numeric ? parseInt(v, 10) : v;
        if (id === "ui_language") applyLanguage(window.__state[id]);
        window.__stateDirty = true;
        close();
    });
    // Close when clicking anywhere outside the widget (list is fixed but still child of root).
    document.addEventListener("mousedown", (e) => {
        if (root.classList.contains("open") && !root.contains(e.target)) close();
    });
    document.addEventListener("keydown", (e) => {
        if (e.key === "Escape" || e.keyCode === 27) close();
    });
    // Reposition on viewport changes while open
    window.addEventListener("resize", () => {
        if (root.classList.contains("open")) positionDrop(root);
    });
    if (panelCol) {
        panelCol.addEventListener("scroll", () => {
            if (root.classList.contains("open")) positionDrop(root);
        }, { passive: true });
    }
    // Also reposition on app scroll/resize (compact mode)
    window.addEventListener("scroll", () => {
        if (root.classList.contains("open")) positionDrop(root);
    }, { passive: true });

    // Seed custom dropdown state from its marked selected item. Native config
    // synchronization may replace this value immediately after DOMReady.
    if (!(id in window.__state)) {
        const selected = root.querySelector(".fdrop-item.selected") || root.querySelector(".fdrop-item");
        if (selected) window.__state[id] = numeric
            ? parseInt(selected.dataset.value, 10)
            : selected.dataset.value;
    }
}

    // ============== Initialize global state ==============
    window.__state = window.__state || {};

    // Bind every control
    [
        // Sliders
        "extra_display_time",
        "mouse_sensitivity", "mouse_accel_factor", "fade_in_time", "fade_out_time",
        "pos_x", "pos_y",
        "font_size", "line_spacing", "max_line_width_percent",
        "text_r", "text_g", "text_b",
        "shadow_offset_x", "outline_thickness",
        "bg_padding_x", "bg_padding_y", "bg_border_radius",
        "font_size_reference_height",
    ].forEach(wireSlider);

    [
        // Switches / checkboxes
        "ui_animations", "mouse_acceleration", "shadow_enabled",
        "outline_enabled", "background_enabled",
    ].forEach(wireCheckbox);

    ["text_alignment"].forEach(wireRadio);
    wireDrop("ui_language", true);                 // numeric (1=EN, 2=AR)
    applyLanguage(window.__state.ui_language || 2);
    wireDrop("custom_font_path", false);           // string (file name)
    wireDrop("fallback_font_path", false);         // string (file name)

    // ============== Color preview (live) ==============
    function updateColorPreview() {
        const r = parseInt(document.getElementById("text_r").value);
        const g = parseInt(document.getElementById("text_g").value);
        const b = parseInt(document.getElementById("text_b").value);
        const p = document.getElementById("text-preview");
        if (p) p.style.background = "rgb(" + r + "," + g + "," + b + ")";
    }
    ["text_r", "text_g", "text_b"].forEach((id) => {
        const el = document.getElementById(id);
        if (el) el.addEventListener("input", updateColorPreview);
    });

    // ============== Caption visibility toggle ==============
    // The native Renderer owns the state. A click only raises a one-shot
    // request; UltralightManager consumes it on the render thread and then
    // sends the authoritative state back through setTranslationVisible().
    const btnHlToggle = document.getElementById("btn-hl-toggle");
    if (btnHlToggle) {
        btnHlToggle.addEventListener("click", (e) => {
            e.preventDefault();
            window.__captionToggleRequested = true;
        });
    }

    // ============== Apply / Reset / Close ==============
    function serializeState() {
        return JSON.stringify(window.__state);
    }
    // Brief visual confirmation after a footer action.
    function flashBtn(el) {
        if (!el) return;
        el.classList.remove("flash");
        void el.offsetWidth;          // force reflow -> restart animation
        el.classList.add("flash");
    }
    // The native side (UltralightManager::Render) polls these flags every
    // frame. Setting them triggers SaveConfig / ResetConfig on the render
    // thread; the native side then calls hlaConfig._clearFlags() to reset.
    document.getElementById("btn-apply").addEventListener("click", () => {
        window.__saveRequested = true;
        console.log("[HLA] APPLY " + serializeState());
        flashBtn(document.getElementById("btn-apply"));
    });
    document.getElementById("btn-reset").addEventListener("click", () => {
        window.__resetRequested = true;
        console.log("[HLA] RESET");
        flashBtn(document.getElementById("btn-reset"));
    });
    // ============== Preview button (loop + live update) ==============
    window.__previewEnabled = false;
    window.__previewText = getPreviewText();
    const btnPreview = document.getElementById("btn-preview");
    if (btnPreview) {
        btnPreview.addEventListener("click", () => {
            const isOn = !window.__previewEnabled;
            window.__previewEnabled = isOn;
            window.__previewText = getPreviewText();
            window.__previewRequested = isOn;
            if (!isOn) window.__previewClear = true;
            btnPreview.classList.toggle("on", isOn);
            console.log("[HLA] PREVIEW " + (isOn ? "ON" : "OFF"));
        });
    }
    document.getElementById("btn-close").addEventListener("click", () => {
        closePanel();
    });

    // ============== Panel slide-in / slide-out ==============
    // Replays the language-side slide-in animation. Called by the native
    // side (UltralightManager::OnPanelOpened) every time the panel opens.
    function panelOpen() {
        const app = document.getElementById("app");
        if (!app) return;
        window.__panelHidden = false;
        document.body.classList.add("panel-open");   // show the blueprint grid
        app.style.transform = "";   // clear any inline override from a previous no-anim open
        window.__noAnim = window.__state && window.__state.ui_animations === false;
        if (app.offsetWidth) {
            app.classList.toggle("compact", app.offsetWidth < 620);
        }
        if (window.__noAnim) {
            app.classList.remove("closing");
            app.classList.remove("open-anim");
            app.style.transform = "translateX(0)"; // show instantly (no animation)
            return;
        }
        app.classList.remove("closing");
        void app.offsetWidth;               // force reflow -> restart animation
        app.classList.remove("open-anim");
        void app.offsetWidth;
        app.classList.add("open-anim");
    }

    // Plays the language-side slide-out animation, then flags the native side
    // (window.__panelHidden) which is polled every frame by
    // UltralightManager::Render and finishes the close.
    function closePanel() {
        const app = document.getElementById("app");
        const finish = () => {
            window.__panelHidden = true;
            document.body.classList.remove("panel-open");   // hide the blueprint grid
            app.style.transform = "";   // fall back to the CSS default (off-screen)
            // Also clear looping preview when panel closes
            if (window.__previewEnabled) {
                window.__previewEnabled = false;
                window.__previewClear = true;
                const btn = document.getElementById("btn-preview");
                if (btn) btn.classList.remove("on");
            }
        };
        if (!app || window.__noAnim) { finish(); return; }
        app.classList.remove("open-anim");
        app.classList.add("closing");
        let done = false;
        const onEnd = (e) => {
            if (done || e.propertyName !== "transform") return;
            done = true;
            app.removeEventListener("transitionend", onEnd);
            app.classList.remove("closing");
            finish();
        };
        app.addEventListener("transitionend", onEnd);
        // Fallback in case the transition event never fires.
        setTimeout(() => {
            if (!done) {
                done = true;
                app.classList.remove("closing");
                finish();
            }
        }, 400);
    }

    // Esc closes the panel (slide-out, then native hides the overlay).
    document.addEventListener("keydown", (e) => {
        if (e.key === "Escape" || e.keyCode === 27) {
            e.preventDefault();
            closePanel();
        }
    });

    // Called from native on every open (see UltralightManager::OnPanelOpened).
    window.hlaPanelOpen = panelOpen;

    // ============== Font pickers ==============
    // The native side (RefreshAllFromConfig) calls window.hlaFonts.setList([...])
    // on page load and after a Reset, with every font file found in
    // {ModDir}/resources (all supported extensions). We fill both dropdowns and
    // re-select the stored values.
    window.hlaFonts = {
        setList: function (names) {
            const ids = ["custom_font_path", "fallback_font_path"];
            ids.forEach((id) => {
                const root = document.getElementById(id);
                if (!root) return;
                const list = root.querySelector(".fdrop-list");
                if (!list) return;
                const stored = (window.__state && window.__state[id])
                    ? String(window.__state[id]) : "";
                const candidates = (names || []).slice();
                // Keep an old INI value (e.g. an absolute path) selectable even
                // if it is not among the resources files.
                if (stored && candidates.indexOf(stored) < 0) candidates.push(stored);
                if (!candidates.length) candidates.push("Cairo-Regular.ttf");  // safe default
                list.innerHTML = "";
                candidates.forEach((name) => {
                    const li = document.createElement("li");
                    li.className = "fdrop-item" + (name === stored ? " selected" : "");
                    li.dataset.value = name;
                    li.textContent = name;
                    list.appendChild(li);
                });
                dropSetValue(id, stored || candidates[0]);
            });
        }
    };

    // ============== Startup banner ==============
    // Native side (UltralightManager) updates the F11 hint label to reflect
    // whether the caption overlay is currently visible. All calls happen
    // on the render thread via EvaluateScript (thread-safe by design).
    window.hlaBanner = {
        setTranslationVisible: function (v) {
            window.__translationVisible = !!v;
            const label = v ? tr("translation.enabled") : tr("translation.disabled");
            // Startup banner hint (top-center).
            const hint = document.getElementById("banner-translation");
            if (hint) hint.textContent = label;
            // Topbar F11 button — shows the live caption state.
            const btn = document.getElementById("btn-hl-toggle");
            if (btn) {
                const label = btn.querySelector("span:first-child");
                if (label) label.textContent = v ? tr("translation.enabled") : tr("translation.disabled");
                btn.classList.toggle("on", !!v);
                btn.setAttribute("aria-pressed", v ? "true" : "false");
            }
        }
    };

    // When the banner's fade-in/hold/fade-out animation finishes, tell the
    // native side (window.__bannerDone, polled in UltralightManager::Render)
    // so it stops rendering the closed overlay entirely (zero cost again).
    const bannerEl = document.getElementById("hud-banner");
    if (bannerEl) {
        bannerEl.addEventListener("animationend", () => {
            window.__bannerDone = true;
        });
    }

    updateColorPreview();
    refreshTitleStats();

    // ============================================================
    // Native -> JS bridge
    // The native side (RefreshAllFromConfig) calls
    //   window.hlaConfig._setValue['<key>'](<value>)
    // after every page load to sync the DOM with the current config.
    // ============================================================

    const SLIDER_KEYS = [
        "extra_display_time",
        "mouse_sensitivity", "mouse_accel_factor", "fade_in_time", "fade_out_time",
        "pos_x", "pos_y",
        "font_size", "line_spacing", "max_line_width_percent",
        "text_r", "text_g", "text_b",
        "shadow_offset_x", "outline_thickness",
        "bg_padding_x", "bg_padding_y", "bg_border_radius",
        "font_size_reference_height",
    ];
    const CHECK_KEYS = [
        "ui_animations", "mouse_acceleration", "shadow_enabled",
        "outline_enabled", "background_enabled",
    ];
    const RADIO_KEYS = ["text_alignment"];
    const SELECT_KEYS = ["ui_language"];
    const STR_SELECT_KEYS = ["custom_font_path", "fallback_font_path"];
    const TEXT_KEYS = [];

    function setControl(id, v, kind) {
        const el = document.getElementById(id);
        if (!el) return;
        if (kind === "slider") {
            el.value = String(v);
            const out = document.getElementById(id + "_out");
            if (out) out.textContent = fmt(id, parseFloat(v));
            paintSlider(el);
            window.__state[id] = parseFloat(v);
            refreshTitleStats();
        } else if (kind === "check") {
            el.checked = !!v;
            window.__state[id] = !!v;
            if (id === "ui_animations") {
                window.__noAnim = !v;
            }
        } else if (kind === "select") {
            const root = document.getElementById(id);
            if (root && root.classList.contains("fdrop")) {
                dropSetValue(id, v);
                window.__state[id] = parseInt(v, 10);
            } else {
                el.value = String(v);
                window.__state[id] = parseInt(v, 10);
            }
            if (id === "ui_language") applyLanguage(window.__state[id]);
        } else if (kind === "strselect") {
            const root = document.getElementById(id);
            if (root && root.classList.contains("fdrop")) {
                dropSetValue(id, v);
                window.__state[id] = String(v);
            } else {
                el.value = String(v);
                window.__state[id] = String(v);
            }
        } else if (kind === "radio") {
            document.querySelectorAll("#" + id + " .seg-btn").forEach((b) =>
                b.classList.toggle("selected", parseInt(b.dataset.value, 10) === parseInt(v, 10)));
            window.__state[id] = parseInt(v, 10);
        } else {  // text
            el.value = String(v);
            window.__state[id] = String(v);
        }
    }

    // Native side hooks a getter via hlaConfig:
    window.hlaConfig = {
        // returns JSON-serialized state to be read by EvaluateScript
        snapshot: () => serializeState(),
        // Called by the native side after a Save/Reset request is consumed,
        // so the same request is not re-triggered on the next frame.
        _clearFlags: () => {
            window.__saveRequested = false;
            window.__resetRequested = false;
            window.__previewRequested = false;
            window.__previewClear = false;
        },
        _setValue: (() => {
            const s = {};
            SLIDER_KEYS.forEach((k) => { s[k] = (v) => setControl(k, v, "slider"); });
            CHECK_KEYS.forEach((k) => { s[k] = (v) => setControl(k, v, "check"); });
            RADIO_KEYS.forEach((k) => { s[k] = (v) => setControl(k, v, "radio"); });
            SELECT_KEYS.forEach((k) => { s[k] = (v) => setControl(k, v, "select"); });
            STR_SELECT_KEYS.forEach((k) => { s[k] = (v) => setControl(k, v, "strselect"); });
            TEXT_KEYS.forEach((k) => { s[k] = (v) => setControl(k, v, "text"); });
            s.shadow_offset_y = () => {};   // dispatched by native, no DOM control
            return s;
        })(),
    };

    console.log("[UL] bindings initialized");
})();
