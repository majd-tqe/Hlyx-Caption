import { defineConfig } from 'vitepress'

export default defineConfig({
  title: 'Hlyx Caption',
  description: 'Arabic-first caption overlay for Half-Life: Alyx — proxy-DLL · HarfBuzz shaping · Ultralight settings',
  lang: 'en-US',
  base: '/Hlyx-Caption/',
  srcDir: '.',
  srcExclude: ['ar/**'],
  cleanUrls: true,
  ignoreDeadLinks: false,

  head: [
    ['link', { rel: 'icon', href: 'data:image/svg+xml,<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100"><text y=".9em" font-size="90">◈</text></svg>' }],
    ['meta', { name: 'theme-color', content: '#ff7a18' }],
    ['link', { rel: 'preconnect', href: 'https://fonts.googleapis.com' }],
    ['link', { rel: 'stylesheet', href: 'https://fonts.googleapis.com/css2?family=Tajawal:wght@400;700&family=IBM+Plex+Sans+Arabic:wght@400;600&display=swap' }],
  ],

  // ── i18n ──────────────────────────────────────────────────────────
  locales: {
    root: {
      label: 'English',
      lang: 'en-US',
      title: 'Hlyx Caption',
      description: 'Arabic-first caption overlay for Half-Life: Alyx — proxy-DLL · HarfBuzz shaping · Ultralight settings',
      themeConfig: {
        nav: [
          { text: 'Guide', link: '/getting-started' },
          { text: 'Architecture', link: '/architecture' },
          { text: 'Configuration', link: '/configuration' },
          { text: 'Contributing', link: '/contributing' },
        ],
      },
    },
  },

  themeConfig: {
    logo: '',
    siteTitle: 'Hlyx Caption Wiki',
    outline: { level: [2, 3], label: 'On this page' },

    nav: [
      { text: 'Guide', link: '/getting-started' },
      { text: 'Architecture', link: '/architecture' },
      { text: 'Configuration', link: '/configuration' },
      { text: 'Contributing', link: '/contributing' },
    ],

    sidebar: [
      {
        text: 'Start Here',
        collapsed: false,
        items: [
          { text: 'Overview', link: '/' },
          { text: 'Getting Started', link: '/getting-started' },
          { text: 'Features', link: '/features' },
          { text: 'FAQ & Troubleshooting', link: '/faq' },
        ],
      },
      {
        text: 'Architecture & Codebase',
        collapsed: false,
        items: [
          { text: 'Architecture', link: '/architecture' },
          { text: 'Caption Pipeline', link: '/modules/caption-pipeline' },
          { text: 'Text Shaping', link: '/modules/text-shaping' },
          { text: 'Core / Bootstrap', link: '/modules/core-bootstrap' },
          { text: 'Hooking & Injection', link: '/modules/hooking' },
          { text: 'Class Diagram', link: '/diagrams/class-diagram' },
          { text: 'Sequence Diagrams', link: '/diagrams/sequences' },
        ],
      },
      {
        text: 'Configuration & Settings',
        collapsed: false,
        items: [
          { text: 'Settings Reference', link: '/configuration' },
          { text: 'Banner Config', link: '/configuration#banner-config-banner-config-h' },
          { text: 'INI Encoding Gotchas', link: '/configuration#encoding-persistence' },
          { text: 'Configuration Module', link: '/modules/configuration' },
        ],
      },
      {
        text: 'UI',
        collapsed: true,
        items: [
          { text: 'Ultralight Panel (HTML/CSS/JS)', link: '/modules/ui-ultralight' },
          { text: 'Adding a Settings Control', link: '/contributing#adding-a-new-setting' },
        ],
      },
      {
        text: 'Localization & Language',
        collapsed: false,
        items: [
          { text: 'Localization Overview', link: '/localization' },
          { text: 'Language Support & Shaping', link: '/language-support' },
          { text: 'Translation Guide', link: '/localization#translation-guide' },
          { text: 'Adding a New Language', link: '/localization#adding-a-new-language' },
        ],
      },
      {
        text: 'Contributor Guides',
        collapsed: false,
        items: [
          { text: 'Development Workflow', link: '/development' },
          { text: 'Adding Features', link: '/contributing' },
          { text: 'Code Style & PR Checklist', link: '/development#code-style-pr-checklist' },
        ],
      },
      {
        text: 'Reference',
        collapsed: true,
        items: [
          { text: 'Module Map', link: '/#module-map' },
          { text: 'Changelog / Versioning', link: '/development#versioning' },
        ],
      },
    ],

    search: {
      provider: 'local',
      options: {
        detailedView: true,
      },
    },

    footer: {
      message: 'Released under project license. Built with VitePress.',
      copyright: 'Copyright © 2026 Hlyx Caption Contributors',
    },

    lastUpdated: {
      text: 'Last updated',
      formatOptions: { dateStyle: 'short', timeStyle: 'short' },
    },
  },

  markdown: {
    theme: { light: 'github-light', dark: 'github-dark' },
    lineNumbers: false,
    config(md) {},
  },

  vite: {
    server: { host: '127.0.0.1', port: 5173 },
  },
})
