// https://nuxt.com/docs/api/configuration/nuxt-config

import vuetify, { transformAssetUrls } from 'vite-plugin-vuetify';

export default defineNuxtConfig({
  devtools: {
    enabled: true,

    timeline: {
      enabled: true
    }
  },
  build: {
    transpile: ['vuetify'],
  },
  modules: [
    (_options, nuxt) => {
      nuxt.hooks.hook('vite:extendConfig', (config) => {
        // @ts-expect-error
        config.plugins.push(vuetify({ autoImport: true }))
      })
    },
    //...
  ],
  vite: {
    build: {
      target: 'esnext'
    },
    esbuild: {
      supported: {
        'top-level-await': true,
      }
    },
    vue: {
        template: {
          transformAssetUrls,
        },
    },
  },
  nitro: {
    esbuild: {
      options: {
        target: 'esnext'
      }
    },
  },
  typescript: {
    tsConfig: {
        compilerOptions: {
            esModuleInterop: true,
        }
    }
  }
})