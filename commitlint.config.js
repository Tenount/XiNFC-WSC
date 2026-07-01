module.exports = {
  extends: ['@commitlint/config-conventional'],
  rules: {
    'type-enum': [
      2,
      'always',
      [
        'feat',
        'fix',
        'docs',
        'style',
        'refactor',
        'test',
        'chore',
        'ci',
        'build',
        'perf',
        'revert',
      ],
    ],
    'scope-enum': [2, 'always', ['xinfc', 'luci-app-xinfc', 'ci', 'docs']],
    'header-max-length': [2, 'always', 100],
    'body-max-line-length': [2, 'always', 120],
    'subject-case': [2, 'always', 'lower-case'],
    'subject-full-stop': [2, 'never', '.'],
  },
};
