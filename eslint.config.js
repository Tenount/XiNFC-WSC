module.exports = [
  {
    languageOptions: {
      ecmaVersion: 'latest',
      sourceType: 'script',
      globals: {
        _: 'readonly',
        N_: 'readonly',
        L: 'readonly',
        E: 'readonly',
        form: 'readonly',
        view: 'readonly',
        fs: 'readonly',
        ui: 'readonly',
        uci: 'readonly',
        network: 'readonly',
        rpc: 'readonly',
        browser: true,
        es2021: true,
      },
      parserOptions: {
        ecmaFeatures: {
          globalReturn: true,
        },
      },
    },
    rules: {
      'no-unused-vars': ['warn', { argsIgnorePattern: '^_|^mode$' }],
      'no-undef': 'warn',
      strict: 'off',
    },
  },
];
