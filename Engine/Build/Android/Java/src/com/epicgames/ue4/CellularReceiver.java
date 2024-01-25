// Copyright Epic Games, Inc. All Rights Reserved.

package com.epicgames.ue4;

import com.epicgames.ue4.Logger;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.NotificationChannel;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import androidx.core.app.NotificationCompat;
import android.content.SharedPreferences;

public class CellularReceiver extends BroadcastReceiver
{
	public void onReceive(Context context, Intent intent)
	{
		SharedPreferences preferences = context.getSharedPreferences("CellularNetworkPreferences", context.MODE_PRIVATE);
		SharedPreferences.Editor editor = preferences.edit();
		editor.putInt("AllowCellular", 1);
		editor.commit();
	}
}