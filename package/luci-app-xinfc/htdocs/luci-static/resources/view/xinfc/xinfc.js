'use strict';
'require view';
'require form';
'require fs';
'require ui';
'require uci';

var AUTH_NAMES = {
  1: 'Open',
  2: 'WPA PSK',
  4: 'Shared',
  8: 'WPA Ent',
  16: 'WPA2 Ent',
  32: 'WPA2 PSK',
};
var ENC_NAMES = { 1: 'None', 2: 'WEP', 4: 'TKIP', 8: 'AES', 12: 'TKIP+AES' };

function fmtBitmask(val, names) {
  if (!val) return '—';
  var bits = [];
  for (var k in names) {
    if ((val & +k) === +k) bits.push(names[k]);
  }
  return bits.length ? bits[bits.length - 1] : '0x' + val.toString(16);
}

function fmtAuth(val) {
  return fmtBitmask(val, AUTH_NAMES);
}
function fmtEnc(val) {
  return fmtBitmask(val, ENC_NAMES);
}
function fmtMac(val) {
  if (typeof val === 'string' && val.indexOf(':') > 0) return val;
  var h = Number(val).toString(16).padStart(12, '0');
  return h.match(/.{2}/g).join(':');
}

function wifiEncToDisplay(enc) {
  if (!enc || enc === 'none') return 'None';
  if (enc.startsWith('wep')) return 'WEP';
  if (enc.includes('tkip') && enc.includes('aes')) return 'TKIP+AES';
  if (
    enc.includes('aes') ||
    enc.includes('ccmp') ||
    enc.includes('sae') ||
    enc.includes('wpa3')
  )
    return 'AES';
  return 'TKIP';
}
function wifiAuthToDisplay(enc) {
  if (!enc || enc === 'none') return 'Open';
  if (enc.startsWith('wep')) return 'WEP/Shared';
  if (enc.includes('sae')) return 'WPA3';
  if (enc.includes('psk2')) return 'WPA2 PSK';
  if (enc.includes('psk')) return 'WPA PSK';
  if (enc.includes('wpa2')) return 'WPA2 Ent';
  return 'WPA Ent';
}

return view.extend({
  load: function () {
    return Promise.all([uci.load('wireless'), uci.load('xinfc')]);
  },

  render: function () {
    let m, s, o;

    var style = document.getElementById('xinfc-layout-style');
    if (!style) {
      style = E(
        'style',
        { id: 'xinfc-layout-style' },
        '#cbi-xinfc { display: flex; flex-wrap: wrap; gap: 16px; } ' +
          '#cbi-xinfc > .cbi-section-title, ' +
          '#cbi-xinfc > .cbi-map-descr { width: 100%; } ' +
          '#cbi-xinfc > .cbi-section { box-sizing: border-box; flex: 1; min-width: 300px; }',
      );
      document.head.appendChild(style);
    }

    m = new form.Map(
      'xinfc',
      _('NFC Configuration'),
      _(
        'Use this page to write Wi-Fi credentials to the <abbr title="Near Field Communication">NFC</abbr> chip via <abbr title="Inter-Integrated Circuit">I2C</abbr>.',
      ),
    );

    var allIfaces = uci.sections('wireless', 'wifi-iface');
    var wifiIfaces = allIfaces.filter(function (f) {
      return !f['.name'].includes('mesh');
    });

    function selIface() {
      var el = document.getElementById('widget.cbid.xinfc.global.iface');
      return el ? el.value : '';
    }
    function ifaceData(name) {
      return wifiIfaces.find(function (f) {
        return f['.name'] === name;
      });
    }

    function stripVer(s) {
      return (s || '').replace(/^.*xinfc version.*\n?/gm, '').trim();
    }

    function readChipAndUpdate() {
      return fs.exec('/usr/sbin/xinfc-wsc', ['--read', '--json']).then(
        function (res) {
          if (res.code === 0 && res.stdout) {
            var c = document.getElementById('nfc-json-cache');
            if (c) c.value = res.stdout;
            updateComparison(selIface());
          } else if (res.code === 0) {
            updateComparison(selIface());
          } else {
            ui.addNotification(
              null,
              E('p', stripVer(res.stderr) || _('No NFC data')),
              'error',
            );
          }
        },
        function (err) {
          ui.addNotification(null, E('p', String(err)), 'error');
        },
      );
    }

    function writeNfc(ifaceName, callback) {
      if (!ifaceName) {
        if (callback) callback('No interface');
        return;
      }
      var f = ifaceData(ifaceName);
      if (!f || !f.ssid) {
        if (callback) callback('No SSID');
        return;
      }
      var ssid = f.ssid,
        pass = f.key || '',
        enc = f.encryption || 'none';
      ui.showModal(_('Writing to NFC...'), [
        E(
          'p',
          { class: 'spinning' },
          _('Please wait while data is being written to the NFC chip.'),
        ),
      ]);
      fs.exec('/usr/sbin/xinfc-wsc', [ssid, pass, enc])
        .then(function (res) {
          ui.hideModal();
          if (res.code === 0) {
            ui.addNotification(null, E('p', _('NFC write OK')), 'info');
            /* Parse JSON from write output to update comparison table */
            if (res.stdout) {
              var c = document.getElementById('nfc-json-cache');
              if (c) c.value = res.stdout;
              updateComparison(selIface());
            }
          } else {
            var msg = _('Failed (code ') + res.code + ')';
            if (res.stderr) msg += ': ' + stripVer(res.stderr);
            ui.addNotification(null, E('p', msg), 'error');
          }
          if (callback) callback();
        })
        .catch(function (err) {
          ui.hideModal();
          ui.addNotification(
            null,
            E('p', _('Error: ') + (err.message || err)),
            'error',
          );
          if (callback) callback();
        });
    }

    /* --- Configuration --- */
    s = m.section(
      form.NamedSection,
      'global',
      'nfc_config',
      _('Configuration'),
    );
    s.addremove = false;

    o = s.option(
      form.Flag,
      'auto_sync',
      _('Auto-sync NFC when Wi-Fi credentials change'),
    );
    o.rmempty = false;
    o.default = '1';

    o = s.option(form.Flag, 'led_feedback', _('Blink router LED on NFC write'));
    o.rmempty = false;
    o.default = '1';

    var ifaceOpt = s.option(form.ListValue, 'iface', _('Wi-Fi Interface'));
    ifaceOpt.value('', _('Select interface...'));
    wifiIfaces.forEach(function (f) {
      ifaceOpt.value(
        f['.name'],
        f['.name'] + (f.ssid ? ' (SSID: ' + f.ssid + ')' : ''),
      );
    });

    var passRevealed = false;

    o = s.option(form.ListValue, 'nfc_action', _('NFC write action'));
    o.value('url', _('Write admin URL'));
    o.value('text', _('Write text message'));
    o.value('clear', _('Clear NFC chip data'));
    o.rmempty = false;
    o.default = 'url';

    o = s.option(form.TextValue, 'nfc_action_text', _('Message text'));
    o.depends('nfc_action', 'text');
    o.placeholder = _('Message text...');
    o.rows = 4;
    o.rmempty = true;
    /* add utf-8 byte counter after render */
    var textId = 'cbid.xinfc.global.nfc_action_text';
    var maxBytes = 150;
    (function initTextCounter() {
      var d = document.getElementById(textId);
      if (!d) {
        setTimeout(initTextCounter, 100);
        return;
      }
      var ta = d.querySelector('textarea') || d.querySelector('input');
      if (!ta) {
        setTimeout(initTextCounter, 100);
        return;
      }
      function utf8len(s) {
        return new Blob([s]).size;
      }
      function charLen(s) {
        try {
          return new TextEncoder().encode(s).length;
        } catch (e) {
          return [...s].length;
        }
      }
      var cnt = document.createElement('div');
      cnt.className = 'text-muted';
      cnt.style.cssText = 'text-align:right;font-size:small';
      cnt.textContent = '0 chars / 0 bytes / ' + maxBytes + ' max';
      d.appendChild(cnt);
      ta.addEventListener('input', function () {
        var bytes = utf8len(this.value);
        var chars = charLen(this.value);
        cnt.textContent =
          chars + ' chars / ' + bytes + ' bytes / ' + maxBytes + ' max';
        if (bytes > maxBytes) cnt.style.color = 'red';
        else cnt.style.color = '';
      });
    })();
    function togglePass() {
      passRevealed = !passRevealed;
      updateComparison(selIface());
    }

    function updateComparison(ifaceName) {
      var f = ifaceData(ifaceName);

      var chipData = {};
      try {
        var cache = document.getElementById('nfc-json-cache');
        if (cache) chipData = JSON.parse(cache.value || '{}');
      } catch (e) {}

      function setCell(id, val) {
        var el = document.getElementById(id);
        if (el) el.textContent = val;
      }

      setCell('nfc-chip-ssid', chipData.ssid || '—');
      setCell('nfc-write-ssid', f ? f.ssid || '—' : '—');

      var chipAuth =
        chipData.auth_type !== undefined ? fmtAuth(chipData.auth_type) : '—';
      var displayEnc = f ? f.encryption || 'none' : 'none';

      setCell('nfc-chip-auth', chipAuth);
      setCell('nfc-write-auth', f ? wifiAuthToDisplay(displayEnc) : '—');

      var chipEnc =
        chipData.encryption_type !== undefined
          ? fmtEnc(chipData.encryption_type)
          : '—';
      var writeEnc = f ? wifiEncToDisplay(displayEnc) : '—';
      setCell('nfc-chip-enc', chipEnc);
      setCell('nfc-write-enc', writeEnc);

      var chipKey = chipData.network_key || '—';
      if (chipKey !== '—' && !passRevealed) chipKey = '••••••••';
      var writeKey = f ? f.key || '—' : '—';
      if (writeKey !== '—' && !passRevealed) writeKey = '••••••••';
      setCell('nfc-chip-key', chipKey);
      setCell('nfc-write-key', writeKey);
    }

    /* --- Diagnostics --- */
    var s_diag = m.section(
      form.NamedSection,
      'global',
      'nfc_config',
      _('Diagnostics & State'),
    );
    s_diag.addremove = false;

    o = s_diag.option(form.DummyValue, '_reader');
    o.renderWidget = function () {
      var cacheInput = E('input', { type: 'hidden', id: 'nfc-json-cache' });

      var table = E('table', { class: 'table cbi-section-table' });
      var tbody = E('tbody');

      var htr = E('tr', { class: 'tr cbi-rowstyle-2' });
      htr.appendChild(E('th', { class: 'th' }, ''));
      htr.appendChild(E('th', { class: 'th' }, _('From NFC')));
      htr.appendChild(E('th', { class: 'th' }, _('From Wi-Fi')));
      tbody.appendChild(htr);

      function makeRow(label, chipId, writeId) {
        var tr = E('tr', { class: 'tr cbi-rowstyle-1' });
        tr.appendChild(E('td', { class: 'th cbi-section-table-cell' }, label));
        tr.appendChild(
          E('td', { class: 'td cbi-section-table-cell', id: chipId }, '—'),
        );
        tr.appendChild(
          E('td', { class: 'td cbi-section-table-cell', id: writeId }, '—'),
        );
        tbody.appendChild(tr);
      }
      makeRow('SSID', 'nfc-chip-ssid', 'nfc-write-ssid');
      makeRow(_('Auth Type'), 'nfc-chip-auth', 'nfc-write-auth');
      makeRow(_('Encryption'), 'nfc-chip-enc', 'nfc-write-enc');
      makeRow(_('Network Key'), 'nfc-chip-key', 'nfc-write-key');

      table.appendChild(tbody);

      var container = E('div', {
        id: 'nfc-table-container',
        style: 'margin-top:8px',
      });
      container.appendChild(table);

      var btnRow = E('div', {
        style:
          'display:flex;gap:6px;margin-bottom:8px;flex-wrap:wrap;align-items:center',
      });

      var readBtn = E(
        'button',
        {
          type: 'button',
          class: 'btn cbi-button cbi-button-action',
          click: function (ev) {
            ev.preventDefault();
            readBtn.disabled = true;
            readBtn.textContent = _('Reading...');
            readChipAndUpdate()
              .then(function () {
                readBtn.disabled = false;
                readBtn.textContent = _('Read from NFC');
              })
              .catch(function (err) {
                readBtn.disabled = false;
                readBtn.textContent = _('Read from NFC');
                ui.addNotification(null, E('p', String(err)), 'error');
              });
          },
        },
        _('Read from NFC'),
      );

      function writeWiFi() {
        var f = ifaceData(selIface());
        if (!f || !f.ssid) {
          ui.addNotification(
            null,
            E('p', _('Select a Wi-Fi interface first')),
            'error',
          );
          return;
        }
        var ssid = f.ssid,
          pass = f.key || '',
          enc = f.encryption || 'none';
        writeBtn.disabled = true;
        fs.exec('/usr/share/xinfc/write-wifi.sh', [ssid, pass, enc])
          .then(function (r) {
            writeBtn.disabled = false;
            var c = document.getElementById('nfc-json-cache');
            if (c) c.value = '{}';
            updateComparison(selIface());
            ui.addNotification(
              null,
              E(
                'p',
                r.code === 0
                  ? _('NFC write OK')
                  : _('Failed: ') + stripVer(r.stderr || ''),
              ),
              r.code === 0 ? 'info' : 'error',
            );
          })
          .catch(function () {
            writeBtn.disabled = false;
          });
      }

      var writeBtn = E(
        'button',
        {
          type: 'button',
          class: 'btn cbi-button cbi-button-positive',
          click: function (ev) {
            ev.preventDefault();
            writeWiFi();
          },
        },
        _('Write to NFC'),
      );

      function updateWriteBtn() {
        if (!writeBtn) return;
        writeBtn.disabled = uci.get('xinfc', 'global', 'auto_sync') === '1';
      }

      var revealBtn = E(
        'button',
        {
          type: 'button',
          class: 'btn cbi-button',
          click: function (ev) {
            togglePass();
          },
        },
        _('Reveal/hide password'),
      );

      var actionLabels = {
        url: _('Write admin URL'),
        text: _('Write custom text'),
        clear: _('Clear NFC'),
      };
      var actionExec = {
        url: function () {
          return fs.exec('/usr/share/xinfc/write-admin.sh');
        },
        text: function () {
          var inpDiv = document.getElementById(
            'cbid.xinfc.global.nfc_action_text',
          );
          var inp = inpDiv ? inpDiv.querySelector('textarea') : null;
          return fs.exec('/usr/share/xinfc/write-text.sh', [
            inp ? inp.value : '',
          ]);
        },
        clear: function () {
          return fs.exec('/usr/share/xinfc/clear-nfc.sh');
        },
      };
      var actionSuccess = {
        url: _('NFC write OK'),
        text: _('NFC write OK'),
        clear: _('NFC chip cleared'),
      };
      function nfcActionWidget() {
        return document.getElementById('widget.cbid.xinfc.global.nfc_action');
      }
      function nfcActionVal() {
        var w = nfcActionWidget();
        return w ? w.value : 'url';
      }
      var _running = false;
      function runAction() {
        if (_running) return;
        var act = nfcActionVal();
        if (act === 'text') {
          var inpDiv = document.getElementById(
            'cbid.xinfc.global.nfc_action_text',
          );
          var inp = inpDiv ? inpDiv.querySelector('textarea') : null;
          var msg = inp ? inp.value : '';
          if (!msg.trim() && !confirm(_('Empty message — write anyway?')))
            return;
          if (new Blob([msg]).size > 150) {
            alert('Text too long: max 150 bytes in UTF-8');
            return;
          }
        }
        if (
          act === 'clear' &&
          !confirm(_('Erase all Wi-Fi credentials from the NFC chip?'))
        )
          return;
        _running = true;
        clearBtn.disabled = true;
        clearBtn.textContent = _('Executing...');
        actionExec[act]()
          .then(function (r) {
            clearBtn.disabled = false;
            clearBtn.textContent = actionLabels[act];
            _running = false;
            var c = document.getElementById('nfc-json-cache');
            if (c) c.value = '{}';
            updateComparison(selIface());
            ui.addNotification(
              null,
              E(
                'p',
                r.code === 0
                  ? actionSuccess[act]
                  : _('Failed: ') + stripVer(r.stderr || ''),
              ),
              r.code === 0 ? 'info' : 'error',
            );
          })
          .catch(function (err) {
            clearBtn.disabled = false;
            clearBtn.textContent = actionLabels[act];
            _running = false;
            ui.addNotification(
              null,
              E('p', _('Error: ') + (err.message || err)),
              'error',
            );
          });
      }
      var clearBtn = E(
        'button',
        {
          type: 'button',
          class: 'btn cbi-button cbi-button-negative',
          click: function (ev) {
            ev.preventDefault();
            runAction();
          },
        },
        _('Clear NFC'),
      );

      function updateActionLabel() {
        if (!clearBtn) return;
        var w = nfcActionWidget();
        clearBtn.textContent = w
          ? actionLabels[w.value] || _('Clear NFC')
          : _('Clear NFC');
      }
      function watchActionText() {
        var w = nfcActionWidget();
        if (!w) return;
        w.addEventListener('change', updateActionLabel);
        var inpDiv = document.getElementById(
          'cbid.xinfc.global.nfc_action_text',
        );
        function syncTextVis() {
          if (!inpDiv) return;
          inpDiv.style.display = w.value === 'text' ? '' : 'none';
        }
        w.addEventListener('change', syncTextVis);
        updateActionLabel();
        syncTextVis();
      }
      (function poll() {
        if (nfcActionWidget()) {
          watchActionText();
          updateWriteBtn();
        } else {
          setTimeout(poll, 200);
        }
      })();

      btnRow.appendChild(readBtn);
      btnRow.appendChild(writeBtn);
      btnRow.appendChild(clearBtn);
      btnRow.appendChild(revealBtn);

      return E('div', { class: 'cbi-value-field' }, [
        btnRow,
        cacheInput,
        container,
      ]);
    };

    ifaceOpt.onchange = function (ev, section_id, value) {
      updateComparison(value);
    };

    setTimeout(function () {
      var saved = uci.get('xinfc', 'global', 'iface');
      if (saved) updateComparison(saved);
    }, 100);

    this.map = m;
    return m.render();
  },
});
