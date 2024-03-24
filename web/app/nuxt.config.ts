// https://nuxt.com/docs/api/configuration/nuxt-config
export default defineNuxtConfig({
  devtools: { enabled: true },
  vite: {
    build: {
      target: 'esnext'
    },
    esbuild: {
      supported: {
        'top-level-await': true,
      }
    }
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
