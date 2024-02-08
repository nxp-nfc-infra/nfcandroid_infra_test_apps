/******************************************************************************
 *
 *  Copyright 2022-2023 NXP
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of NXP nor the names of its contributors may be used
 * to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/
package com.nxp.test.emvco;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.nfc.NfcAdapter;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.TextUtils;
import android.text.style.ForegroundColorSpan;
import android.util.Log;
import android.view.View;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.Switch;
import android.widget.TextView;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.AppCompatButton;
import androidx.appcompat.widget.Toolbar;
import androidx.core.content.ContextCompat;
import com.nxp.emvco.INxpEMVCoClientCallback;
import com.nxp.emvco.NxpConfigType;
import com.nxp.emvco.NxpEmvcoEvent;
import com.nxp.emvco.NxpEmvcoStatus;
import com.nxp.emvco.NxpProfileDiscovery;
import vendor.nxp.emvco.NxpDiscoveryMode;

public class EMVCoModeSwitchActivity
    extends AppCompatActivity implements INxpEMVCoClientCallback {

  private static final String TAG = EMVCoModeSwitchActivity.class.getName();
  private static int mNfcState = NfcAdapter.STATE_OFF;
  private TextView mResult;
  private NxpProfileDiscovery mProfileDiscovery;
  private NfcAdapter mNfcAdapter;
  private Switch mEMVCoSwitch;
  private CheckBox mTypeACheckBox, mTypeBCheckBox, mTypeFCheckBox;
  private AppCompatButton mNfcStatusButton, mEMVCoStatusButton;
  static int mTagTypeConfiguration = 0;
  private static final int NFC_A_PASSIVE_POLL_MODE = 0;
  private static final int NFC_B_PASSIVE_POLL_MODE = 1;
  private static final int NFC_F_PASSIVE_POLL_MODE = 2;

  enum NfcStatus {
    EMVCo_ON,
    EMVCo_OFF,
    NFC_ON,
    NFC_OFF;
  }
  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);
    Toolbar toolbar = findViewById(R.id.toolbar);
    setSupportActionBar(toolbar);
    SpannableString emvcoTitle = new SpannableString("EMVCo");
    SpannableString modeTitle = new SpannableString("Mode");
    SpannableString switchTitle = new SpannableString("Switch");

    emvcoTitle.setSpan(new ForegroundColorSpan(ContextCompat.getColor(
                           getApplicationContext(), R.color.orange)),
                       0, emvcoTitle.length(),
                       Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
    modeTitle.setSpan(new ForegroundColorSpan(ContextCompat.getColor(
                          getApplicationContext(), R.color.blue)),
                      0, modeTitle.length(),
                      Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
    switchTitle.setSpan(new ForegroundColorSpan(ContextCompat.getColor(
                            getApplicationContext(), R.color.green)),
                        0, switchTitle.length(),
                        Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);

    getSupportActionBar().setTitle(
        TextUtils.concat(emvcoTitle, modeTitle, switchTitle));
    ((TextView)toolbar.getChildAt(0)).setTextSize(25);
    mResult = findViewById(R.id.text_view);
    mEMVCoSwitch = (Switch)findViewById(R.id.emvco_switch);
    mTypeACheckBox = (CheckBox)findViewById(R.id.emvco_type_a);
    mTypeBCheckBox = (CheckBox)findViewById(R.id.emvco_type_b);
    mTypeFCheckBox = (CheckBox)findViewById(R.id.emvco_type_f);
    mNfcStatusButton = (AppCompatButton)findViewById(R.id.nfc_status);
    mEMVCoStatusButton = (AppCompatButton)findViewById(R.id.emvco_status);
    mNfcStatusButton.setEnabled(false);
    mEMVCoStatusButton.setEnabled(false);
    mNfcAdapter = NfcAdapter.getDefaultAdapter(this);
    mProfileDiscovery = NxpProfileDiscovery.getInstance(this);
  }

  @Override
  protected void onStart() {
    super.onStart();
    mProfileDiscovery.registerEventListener(this);
    IntentFilter nfcntentFilter =
        new IntentFilter(NfcAdapter.ACTION_ADAPTER_STATE_CHANGED);
    registerReceiver(mReceiver, nfcntentFilter);
  }
  protected void onResume() {
    super.onResume();
    updateUserInterface();
    mEMVCoSwitch.setOnCheckedChangeListener(onCheckedChangeListener);
    mTypeACheckBox.setOnCheckedChangeListener(onCheckedChangeListener);
    mTypeBCheckBox.setOnCheckedChangeListener(onCheckedChangeListener);
    mTypeFCheckBox.setOnCheckedChangeListener(onCheckedChangeListener);
  }

  synchronized void updateUI(NfcStatus nfcStatus) {
    switch (nfcStatus) {
    case EMVCo_ON:
      mEMVCoStatusButton.setBackgroundColor(Color.GREEN);
      mEMVCoStatusButton.setText("EMVCo ON");
      mEMVCoSwitch.setChecked(true);
      break;
    case EMVCo_OFF:
      mEMVCoStatusButton.setBackgroundColor(Color.RED);
      mEMVCoStatusButton.setText("EMVCo OFF");
      mEMVCoSwitch.setChecked(false);
      break;
    case NFC_ON:
      mNfcStatusButton.setBackgroundColor(Color.GREEN);
      mNfcStatusButton.setText("NFC ON");
      break;
    case NFC_OFF:
      mNfcStatusButton.setBackgroundColor(Color.RED);
      mNfcStatusButton.setText("NFC OFF");
      break;
    }
  }
  private void updateUserInterface() {
    int profileMode = mProfileDiscovery.getCurrentDiscoveryMode();
    Log.d(TAG, "updateUserInterface profileMode:" + profileMode);
    if (profileMode == NxpDiscoveryMode.NFC) {
      updateUI(NfcStatus.NFC_ON);
      updateUI(NfcStatus.EMVCo_OFF);
    } else if (profileMode == NxpDiscoveryMode.EMVCO) {
      updateUI(NfcStatus.EMVCo_ON);
      updateUI(NfcStatus.NFC_OFF);
    } else {
      updateUI(NfcStatus.EMVCo_OFF);
      updateUI(NfcStatus.NFC_OFF);
    }
  }
  static void setRFTechnologyMode(int modeType, boolean isSet) {
    Log.d(TAG, "Before set mTagTypeConfiguration" + mTagTypeConfiguration);
    if (isSet) {
      mTagTypeConfiguration = 1 << modeType | mTagTypeConfiguration;
    } else {
      mTagTypeConfiguration = ~(1 << modeType) & mTagTypeConfiguration;
    }
    Log.d(TAG, "after set mTagTypeConfiguration" + mTagTypeConfiguration);
  }

  private void onEMVCoButtonClick(boolean isChecked) {
    Log.d(TAG, "onEMVCoButtonClick "
                   + "isChecked:" + isChecked);
    if (isChecked) {
      mResult.setText("Enabling EMVCO!!");
      byte pollProfileSelectVal = 0b00000010;
      int keyValue = NxpConfigType.POLL_PROFILE_SEL.getValue();
      int numKeyDigits = Integer.toString(keyValue).length();
      Log.d(TAG, "numKeyDigits: " + numKeyDigits);
      mProfileDiscovery.setByteConfig(keyValue, numKeyDigits,
                                      pollProfileSelectVal);
      mProfileDiscovery.setEMVCoMode(mTagTypeConfiguration, true);
    } else {
      mResult.setText("Disabling EMVCO!!");
      mProfileDiscovery.setEMVCoMode(mTagTypeConfiguration, false);
    }
  }
  private void onTypeAButtonClick(boolean isChecked) {
    if (isChecked) {
      setRFTechnologyMode(NFC_A_PASSIVE_POLL_MODE, true);
    } else {
      setRFTechnologyMode(NFC_A_PASSIVE_POLL_MODE, false);
    }
  }
  private void onTypeBButtonClick(boolean isChecked) {
    if (isChecked) {
      setRFTechnologyMode(NFC_B_PASSIVE_POLL_MODE, true);
    } else {
      setRFTechnologyMode(NFC_B_PASSIVE_POLL_MODE, false);
    }
  }

  private void onTypeFButtonClick(boolean isChecked) {
    if (isChecked) {
      setRFTechnologyMode(NFC_F_PASSIVE_POLL_MODE, true);
    } else {
      setRFTechnologyMode(NFC_F_PASSIVE_POLL_MODE, false);
    }
  }

  private final CompoundButton.OnCheckedChangeListener onCheckedChangeListener =
      new CompoundButton.OnCheckedChangeListener() {
        @Override
        public void onCheckedChanged(CompoundButton buttonView,
                                     boolean isChecked) {
          final int buttonId = buttonView.getId();
          switch (buttonId) {
          case R.id.emvco_switch:
            onEMVCoButtonClick(isChecked);
            break;
          case R.id.emvco_type_a:
            onTypeAButtonClick(isChecked);
            break;
          case R.id.emvco_type_b:
            onTypeBButtonClick(isChecked);
            break;
          case R.id.emvco_type_f:
            onTypeFButtonClick(isChecked);
            break;
          }
        }
      };

  private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
    @Override
    public void onReceive(Context context, Intent intent) {
      String action = intent.getAction();
      if (NfcAdapter.ACTION_ADAPTER_STATE_CHANGED.equals(action)) {
        mNfcState = intent.getIntExtra(NfcAdapter.EXTRA_ADAPTER_STATE,
                                       NfcAdapter.STATE_OFF);
        Log.d(TAG, "Recived NFC State Change as " + mNfcState);
        if (mNfcState == NfcAdapter.STATE_OFF) {
          mResult.setText("NFC Mode is OFF");
          updateUI(NfcStatus.NFC_OFF);
        } else if (mNfcState == NfcAdapter.STATE_ON) {
          mResult.setText("NFC Mode is ON");
          updateUI(NfcStatus.NFC_ON);
        }
      }
    }
  };
  @Override
  public void sendData(byte[] data) {
    Log.e(TAG, "Received EMVCo data");
  }
  @Override
  public void sendEvent(NxpEmvcoEvent event, NxpEmvcoStatus status) {
    Log.e(TAG, "Received EMVCo Event:" + event);
    Log.e(TAG, "Received EMVCo status:" + status);
    new Handler(Looper.getMainLooper()).post(new Runnable() {
      @Override
      public void run() {
        Log.d(TAG, "update UI from updateResult");
        updateResult(event, status);
      }
    });
  }
  private void updateResult(NxpEmvcoEvent event, NxpEmvcoStatus status) {
    switch (event) {
    case EMVCO_OPEN_CHNL_CPLT_EVT:
      Log.e(TAG, "EMVCO_OPEN_CHNL_CPLT_EVT");
      mResult.setText("EMVCo HAL Open Success");
      break;
    case EMVCO_OPEN_CHNL_ERROR_EVT:
      Log.e(TAG, "EMVCO_OPEN_CHNL_ERROR_EVT");
      mResult.setText("EMVCo HAL Open failed");
      break;
    case EMVCO_CLOSE_CHNL_CPLT_EVT:
      Log.e(TAG, "EMVCO_CLOSE_CHNL_CPLT_EVT");
      if (status == NxpEmvcoStatus.EMVCO_STATUS_OK) {
        updateUI(NfcStatus.EMVCo_OFF);
        mResult.setText("EMVCo HAL Close Success");
      } else {
        mResult.setText("EMVCo HAL Close failed");
      }
      break;
    case EMVCO_POOLING_START_EVT:
      Log.e(TAG, "EMVCO_POOLING_START_EVT");
      if (status == NxpEmvcoStatus.EMVCO_STATUS_OK) {
        mResult.setText("Starting EMVCo Mode");
      } else {
        mEMVCoSwitch.setChecked(false);
        mResult.setText(
            "EMVCo start failed. Select valid technolgy/technolgy combination to poll");
      }
      break;
    case EMVCO_POLLING_STARTED_EVT:
      Log.e(TAG, "EMVCO_POLLING_STARTED_EVT");
      if (status == NxpEmvcoStatus.EMVCO_STATUS_OK) {
        updateUI(NfcStatus.EMVCo_ON);
        mResult.setText("EMVCo Poll Activated");
      } else {
        mEMVCoSwitch.setChecked(false);
        mResult.setText(
            "EMVCo poll failed. Select valid technolgy/technolgy combination to poll");
      }
      break;
    case EMVCO_POLLING_STOP_EVT:
      Log.e(TAG, "EMVCO_POLLING_STOP_EVT");
      if (status == NxpEmvcoStatus.EMVCO_STATUS_OK) {
        updateUI(NfcStatus.EMVCo_OFF);
        mResult.setText("EMVCo Mode de-activated");
      } else {
        mResult.setText("EMVCo Mode de-activate failed");
      }
      break;
    case EMVCO_UN_SUPPORTED_CARD_EVT:
      mResult.setText("Un supported EMVCo Card detected");
      break;
    }
  }

  @Override
  protected void onStop() {
    super.onStop();
    unregisterReceiver(mReceiver);
  }
}
