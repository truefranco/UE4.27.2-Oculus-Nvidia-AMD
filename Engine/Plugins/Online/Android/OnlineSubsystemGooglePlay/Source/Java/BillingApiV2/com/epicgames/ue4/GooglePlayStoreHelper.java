// Copyright Epic Games, Inc. All Rights Reserved.
//This file needs to be here so the "ant" build step doesnt fail when looking for a /src folder.

package com.epicgames.ue4;

import android.content.Intent;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.billingclient.api.BillingClient;
import com.android.billingclient.api.BillingClientStateListener;
import com.android.billingclient.api.BillingFlowParams;
import com.android.billingclient.api.BillingResult;
import com.android.billingclient.api.ConsumeParams;
import com.android.billingclient.api.ConsumeResponseListener;
import com.android.billingclient.api.ProductDetails;
import com.android.billingclient.api.ProductDetailsResponseListener;
import com.android.billingclient.api.Purchase;
import com.android.billingclient.api.PurchasesResponseListener;
import com.android.billingclient.api.PurchasesUpdatedListener;
import com.android.billingclient.api.QueryProductDetailsParams;
import com.android.billingclient.api.QueryPurchasesParams;
import com.android.vending.billing.util.Base64; //WMM should use different base64 here.
import java.util.ArrayList;
import java.util.List;

public class GooglePlayStoreHelper implements StoreHelper
{
	// Our IAB helper interface provided by google.
	private BillingClient mBillingClient;

	// Flag that determines whether the store is ready to use.
	private boolean bIsIapSetup;

	// Output device for log messages.
	private Logger Log;

	// Cache access to the games activity.
	private GameActivity gameActivity;

	private final int UndefinedFailureResponse = -1;

	private int outstandingConsumeRequests = 0;

	private boolean bIsConsumeComplete = false;

	static private class GooglePlayProductDescription
	{
		// Product offer id 
		public String id;
		// Product friendly name
		public String title;
		// Product description
		public String description;
		// Currency friendly string 
		public String price;
		// Raw price in currency units
		public Float priceRaw;
		// Local currency code
		public String currencyCode;
	}

	public GooglePlayStoreHelper(String InProductKey, GameActivity InGameActivity, final Logger InLog)
	{
		// IAP is not ready to use until the service is instantiated.
		bIsIapSetup = false;

		Log = InLog;
		Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::GooglePlayStoreHelper");

		gameActivity = InGameActivity;
	
		PurchasesUpdatedListener purchasesUpdatedListener = new PurchasesUpdatedListener() {
			@Override
			public void onPurchasesUpdated(@NonNull BillingResult billingResult, @Nullable List<Purchase> purchases) 
			{
				if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK && purchases != null) 
				{
					for (Purchase purchase : purchases) 
					{
						onPurchaseResult(billingResult, purchase);
					}
				} 
				else
				{
					Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::UserCancelled Purchase " + billingResult.getDebugMessage());
					nativePurchaseComplete(billingResult.getResponseCode(), "", "", "", "");
					// Handle an error caused by a user cancelling the purchase flow.
				}
			}
		};

		mBillingClient = BillingClient.newBuilder(gameActivity).setListener(purchasesUpdatedListener).enablePendingPurchases().build();

		mBillingClient.startConnection(new BillingClientStateListener() {
			@Override
			public void onBillingSetupFinished(@NonNull BillingResult billingResult) 
			{
				if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK) 
				{
					bIsIapSetup = true;
					Log.debug("In-app billing supported for " + gameActivity.getPackageName());
				}
				else
				{
					Log.debug("In-app billing NOT supported for " + gameActivity.getPackageName() + " error " + billingResult.getResponseCode());
				}
			}

			@Override
			public void onBillingServiceDisconnected() {
				// Try to restart the connection on the next request to
				// Google Play by calling the startConnection() method.
				bIsIapSetup = false;
			}
		});
	}

	///////////////////////////////////////////////////////
	// The StoreHelper interfaces implementation for Google Play Store.

	/**
	 * Determine whether the store is ready for purchases.
	 */
	public boolean IsAllowedToMakePurchases()
	{
		Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::IsAllowedToMakePurchases");
		return bIsIapSetup;
	}

	/**
	 * Query product details for the provided skus
	 */
	public boolean QueryInAppPurchases(String[] InProductIDs)
	{
		Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::QueryInAppPurchases");

		if (InProductIDs.length > 0)
		{
			List<QueryProductDetailsParams.Product> productsList = new ArrayList<>(InProductIDs.length);

			for (String productId : InProductIDs)
			{
				Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::QueryInAppPurchases - Querying " + productId);
				QueryProductDetailsParams.Product.Builder builder = QueryProductDetailsParams.Product.newBuilder();
				builder.setProductType(BillingClient.ProductType.INAPP);
				builder.setProductId(productId);
				productsList.add(builder.build());
			}

			Log.debug("[GooglePlayStoreHelper] - NumSkus: " + productsList.size());

			QueryProductDetailsParams.Builder builder = QueryProductDetailsParams.newBuilder();
			builder.setProductList(productsList);
			
			mBillingClient.queryProductDetailsAsync (builder.build(), new ProductDetailsResponseListener() {
					@Override
					public void onProductDetailsResponse(@NonNull BillingResult billingResult, @NonNull List<ProductDetails> productDetailsList) {
						// Process the result.
						int response = billingResult.getResponseCode();

						if (response == BillingClient.BillingResponseCode.OK)
						{
							Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::QueryInAppPurchases - Response " + response + " SkuDetails:" + productDetailsList.toString());

							ArrayList<GooglePlayProductDescription> productDescriptions = new ArrayList<>();
							for (ProductDetails product : productDetailsList)
							{
								GooglePlayProductDescription newDescription = new GooglePlayProductDescription();
								newDescription.id = product.getProductId();
								newDescription.title = product.getTitle();
								newDescription.description = product.getDescription();
								ProductDetails.OneTimePurchaseOfferDetails OfferDetails = product.getOneTimePurchaseOfferDetails();
								newDescription.price = OfferDetails.getFormattedPrice();
								double priceRaw = OfferDetails.getPriceAmountMicros() / 1000000.0;
								newDescription.priceRaw = (float) priceRaw;
								newDescription.currencyCode = OfferDetails.getPriceCurrencyCode();
								productDescriptions.add(newDescription);
							}

							// Should we send JSON, or somehow serialize this so we can have random access fields?
							ArrayList<String> productIds = new ArrayList<>();
							ArrayList<String> titles = new ArrayList<>();
							ArrayList<String> descriptions = new ArrayList<>();
							ArrayList<String> prices = new ArrayList<>();
							ArrayList<Float> pricesRaw = new ArrayList<>();
							ArrayList<String> currencyCodes = new ArrayList<>();

							for (GooglePlayProductDescription product : productDescriptions)
							{
								productIds.add(product.id);
								Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::QueryInAppPurchases - Parsing details for: " + product.id);

								titles.add(product.title);
								Log.debug("[GooglePlayStoreHelper] - title: " + product.title);

								descriptions.add(product.description);
								Log.debug("[GooglePlayStoreHelper] - description: " + product.description);

								prices.add(product.price);
								Log.debug("[GooglePlayStoreHelper] - price: " + product.price);

								pricesRaw.add(product.priceRaw);
								Log.debug("[GooglePlayStoreHelper] - price_amount_micros: " + product.priceRaw);

								currencyCodes.add(product.currencyCode);
								Log.debug("[GooglePlayStoreHelper] - price_currency_code: " + product.currencyCode);

							}

							float[] pricesRawPrimitive = new float[pricesRaw.size()];
							for (int i = 0; i < pricesRaw.size(); i++)
							{
								pricesRawPrimitive[i] = pricesRaw.get(i);
							}

							Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::QueryInAppPurchases " + productIds.size() + " items - Success!");
							nativeQueryComplete(BillingClient.BillingResponseCode.OK, productIds.toArray(new String[0]), titles.toArray(new String[0]), descriptions.toArray(new String[0]), prices.toArray(new String[0]), pricesRawPrimitive, currencyCodes.toArray(new String[0]));
						}
						else
						{
							Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::QueryInAppPurchases - Failed with: " + response);
							// If anything fails right now, stop immediately, not sure how to reconcile partial success/failure
							nativeQueryComplete(response, null, null, null, null, null, null);
						}

						Log.debug("[GooglePlayStoreHelper] - nativeQueryComplete done!");
					}
				}
			);
		}
		else
		{
			// nothing to query
			Log.debug("[GooglePlayStoreHelper] - no products given to query");
			nativeQueryComplete(UndefinedFailureResponse, null, null, null, null, null, null);
			return false;
		}

		return true;
	}

	/**
	 * Start the purchase flow for a particular sku
	 */
	public boolean BeginPurchase(String ProductID, final String ObfuscatedAccountId)
	{
		List<QueryProductDetailsParams.Product> productToPurchase = new ArrayList<>();
		QueryProductDetailsParams.Product.Builder productBuilder = QueryProductDetailsParams.Product.newBuilder();
		productBuilder.setProductType(BillingClient.ProductType.INAPP);
		productBuilder.setProductId(ProductID);
		productToPurchase.add(productBuilder.build());
		QueryProductDetailsParams.Builder queryBuilder = QueryProductDetailsParams.newBuilder();
		queryBuilder.setProductList(productToPurchase);
		mBillingClient.queryProductDetailsAsync(queryBuilder.build(), new ProductDetailsResponseListener() {
			@Override
			public void onProductDetailsResponse(@NonNull BillingResult billingResult, @NonNull List<ProductDetails> productDetailsList) 
			{
				int response = billingResult.getResponseCode();
				if (response == BillingClient.BillingResponseCode.OK)
				{
					List<BillingFlowParams.ProductDetailsParams> productsToPurchase = new ArrayList<>();
					for (ProductDetails productDetails : productDetailsList)
					{
						BillingFlowParams.ProductDetailsParams.Builder builder = BillingFlowParams.ProductDetailsParams.newBuilder();
						builder.setProductDetails(productDetails);
						productsToPurchase.add(builder.build());
						Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::BeginPurchase - Adding to billing flow " + productDetails.getProductId());
					}

					BillingFlowParams.Builder flowParams = BillingFlowParams.newBuilder().setProductDetailsParamsList(productsToPurchase);

					if (ObfuscatedAccountId != null)
					{
						flowParams = flowParams.setObfuscatedAccountId(ObfuscatedAccountId);
					}

					response = mBillingClient.launchBillingFlow(gameActivity, flowParams.build()).getResponseCode();
				}
				if (response != BillingClient.BillingResponseCode.OK)
				{
					Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::BeginPurchase - Failed! " + TranslateServerResponseCode(response));
					nativePurchaseComplete(UndefinedFailureResponse, "", "", "", "");
				}
			}
		});
		return true;
	}

	/**
	 * Restore previous purchases the user may have made.
	 */
	public boolean RestorePurchases(final String[] InProductIDs, final boolean[] bConsumable)
	{
		Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::RestorePurchases");

		QueryPurchasesParams PurchaseParams = QueryPurchasesParams.newBuilder().setProductType(BillingClient.ProductType.INAPP).build();
		mBillingClient.queryPurchasesAsync(PurchaseParams, new PurchasesResponseListener() {
			@Override
			public void onQueryPurchasesResponse(@NonNull BillingResult billingResult, @NonNull List<Purchase> ownedProducts)
			{
				int responseCode = billingResult.getResponseCode();

				if (responseCode != BillingClient.BillingResponseCode.OK)
				{
					nativeRestorePurchasesComplete(responseCode, null, null, null, null);
					Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::RestorePurchases - Failed to collect existing purchases");
					return;
				}
				Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::RestorePurchases - User has previously purchased " + ownedProducts.size() + " inapp products");

				final ArrayList<String> ownedSkus = new ArrayList<>();
				final ArrayList<String> signatureList = new ArrayList<>();

				Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::RestorePurchases - Now consuming any purchases that may have been missed.");
				final ArrayList<String> productTokens = new ArrayList<>();
				final ArrayList<String> receipts = new ArrayList<>();
				final ArrayList<Integer> cachedErrorResponses = new ArrayList<>();

				bIsConsumeComplete = false;
				// @todo possible bug, restores ALL ownedSkus, not just those provided by caller in RestoreProductIDs
				for (Purchase ownedProduct : ownedProducts)
				{
					productTokens.add(ownedProduct.getPurchaseToken());

					boolean bTryToConsume = false;

					// This is assuming that all purchases should be consumed. Consuming a purchase that is meant to be a one-time purchase makes it so the
					// user is able to buy it again. Also, it makes it so the purchase will not be able to be restored again in the future.

					for (int Idy = 0; Idy < InProductIDs.length; Idy++)
					{
						if (ownedProduct.getProducts().contains(InProductIDs[Idy]))
						{
							if (Idy < bConsumable.length)
							{
								bTryToConsume = bConsumable[Idy];
								Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::RestorePurchases - Found Consumable Flag for Product " + ownedProduct.getProducts().get(0) + " bConsumable = " + bTryToConsume);
							}
							break;
						}
					}

					final Purchase constProduct = ownedProduct;
					if (bTryToConsume)
					{
						Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::RestorePurchases - Attempting to consume " + ownedProduct.getProducts().get(0));
						ConsumeParams consumeParams = ConsumeParams.newBuilder().setPurchaseToken(ownedProduct.getPurchaseToken()).build();
						mBillingClient.consumeAsync(consumeParams, new ConsumeResponseListener() {
							@Override
							public void onConsumeResponse(@NonNull BillingResult billingResult, @NonNull String purchaseToken)
							{
								if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK)
								{
									// How do we get purchase here.. need to figure out a way to persist cachedResponse and receipts
									Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::RestorePurchases - Purchase restored for " + constProduct.getProducts().get(0));
									String receipt = Base64.encode(constProduct.getOriginalJson().getBytes());
									receipts.add(receipt);

									ownedSkus.add(constProduct.getProducts().get(0));
									signatureList.add(constProduct.getSignature());
								}
								else
								{
									Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::RestorePurchases - consumePurchase failed for " + constProduct.getProducts().get(0) + " with error " + billingResult.getResponseCode());
									receipts.add("");
									cachedErrorResponses.add(billingResult.getResponseCode());
								}
								outstandingConsumeRequests--;

								if (outstandingConsumeRequests <= 0)
								{
									Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::RestorePurchases - Success!");
									int finalResponse = (cachedErrorResponses.size() > 0) ? cachedErrorResponses.get(0) : BillingClient.BillingResponseCode.OK;
									nativeRestorePurchasesComplete(finalResponse, ownedSkus.toArray(new String[0]), productTokens.toArray(new String[0]), receipts.toArray(new String[0]), signatureList.toArray(new String[0]));
									bIsConsumeComplete = true;
								}
							}
						});

						outstandingConsumeRequests++;
					}
					else
					{
						// How do we get purchase here.. need to figure out a way to persist cachedResponse and receipts
						Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::RestorePurchases - Purchase restored for " + constProduct.getProducts().get(0));
						String receipt = Base64.encode(constProduct.getOriginalJson().getBytes());
						receipts.add(receipt);

						ownedSkus.add(constProduct.getProducts().get(0));
						signatureList.add(constProduct.getSignature());
					}
				}

				// There was nothing to consume, so we are done!
				if (outstandingConsumeRequests <= 0 && !bIsConsumeComplete)
				{
					Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::RestorePurchases - Success!");
					nativeRestorePurchasesComplete(BillingClient.BillingResponseCode.OK, ownedSkus.toArray(new String[0]), productTokens.toArray(new String[0]), receipts.toArray(new String[0]), signatureList.toArray(new String[0]));
				}
			}
		});

		return true;
	}

	public boolean QueryExistingPurchases()
	{
		Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::QueryExistingPurchases");

		QueryPurchasesParams PurchaseParams = QueryPurchasesParams.newBuilder().setProductType(BillingClient.ProductType.INAPP).build();
		mBillingClient.queryPurchasesAsync(PurchaseParams, new PurchasesResponseListener() {
			@Override
			public void onQueryPurchasesResponse(@NonNull BillingResult billingResult, @NonNull List<Purchase> ownedProducts)
			{
				int responseCode = billingResult.getResponseCode();
				if (responseCode != BillingClient.BillingResponseCode.OK)
				{
					Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::QueryExistingPurchases - Failed to collect existing purchases" + TranslateServerResponseCode(responseCode));
					nativeQueryExistingPurchasesComplete(responseCode, null, null, null, null);
					return;
				}

				Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::QueryExistingPurchases - User has previously purchased " + ownedProducts.size() + " inapp products");

				ArrayList<String> ownedSkus = new ArrayList<>();
				ArrayList<String> signatureList = new ArrayList<>();
				ArrayList<String> productTokens = new ArrayList<>();
				ArrayList<String> receipts = new ArrayList<>();

				for (Purchase purchase : ownedProducts)
				{
					productTokens.add(purchase.getPurchaseToken());
					ownedSkus.add(purchase.getProducts().get(0));
					signatureList.add(purchase.getSignature());
					String receipt = Base64.encode(purchase.getOriginalJson().getBytes());
					receipts.add(receipt);
				}

				Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::QueryExistingPurchases - Success!");
				nativeQueryExistingPurchasesComplete(responseCode, ownedSkus.toArray(new String[0]), productTokens.toArray(new String[0]), receipts.toArray(new String[0]), signatureList.toArray(new String[0]));
			}
		});
		return true;
	}

	public void ConsumePurchase(String purchaseToken)
	{
		Log.debug("[GooglePlayStoreHelper] - Beginning ConsumePurchase: " + purchaseToken);

		ConsumeParams consumeParams = ConsumeParams.newBuilder().setPurchaseToken(purchaseToken).build();
		mBillingClient.consumeAsync(consumeParams, new ConsumeResponseListener() {
			@Override
			public void onConsumeResponse(@NonNull BillingResult billingResult, @NonNull String token) {
				if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK)
				{
					Log.debug("[GooglePlayStoreHelper] - ConsumePurchase success");
				}
				else
				{
					Log.debug("[GooglePlayStoreHelper] - ConsumePurchase failed with error " + TranslateServerResponseCode(billingResult.getResponseCode()));
				}
			}
		});
	}

	public boolean onActivityResult(int requestCode, int resultCode, Intent data)
	{
		Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::onActivityResult unimplemented on BillingApiV2");
		return false;
	}

	///////////////////////////////////////////////////////
	// Game Activity/Context driven methods we need to listen for.

	/**
	 * On Destory we should unbind our IInAppBillingService service
	 */
	public void onDestroy()
	{
		Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::onDestroy");

		if (mBillingClient != null)
		{
			mBillingClient = null; //WMM ???
		}
	}

	/**
	 * Route taken by a successful Purchase workflow
	*/
	public void onPurchaseResult(BillingResult billingResult, Purchase purchase)
	{
		Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::onPurchaseResult");

		if (billingResult == null)
		{
			Log.debug("Null data in purchase activity result.");
			nativePurchaseComplete(UndefinedFailureResponse, "", "", "", "");
			return;
		}

		int responseCode = billingResult.getResponseCode();
		if (responseCode == BillingClient.BillingResponseCode.OK)
		{
			Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::onActivityResult - Processing purchase result. Response Code: " + TranslateServerResponseCode(responseCode));
			Log.debug("Purchase data: " + purchase.toString());
			Log.debug("Data signature: " + purchase.getSignature());

			String sku = purchase.getProducts().get(0);
			if(purchase.getPurchaseState() == Purchase.PurchaseState.PURCHASED)
			{
				String receipt = Base64.encode(purchase.getOriginalJson().getBytes());
				nativePurchaseComplete(BillingClient.BillingResponseCode.OK, sku, purchase.getPurchaseToken(), receipt, purchase.getSignature());

			}
			else
			{
				// This is a pending purchase.. need to hold off on reporting as purchase to game until it actually occurs.
				// Special case - if pending purchase is USER_CANCELLED but the SKU is returned, then it is deferred
				nativePurchaseComplete(BillingClient.BillingResponseCode.USER_CANCELED, sku, "", "", "");
			}
		}
		else if (responseCode == BillingClient.BillingResponseCode.USER_CANCELED)
		{
			Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::onActivityResult - Purchase canceled." + TranslateServerResponseCode(responseCode));
			nativePurchaseComplete(BillingClient.BillingResponseCode.USER_CANCELED, "", "", "", "");
		}
		else
		{
			Log.debug("[GooglePlayStoreHelper] - GooglePlayStoreHelper::onActivityResult - Purchase failed. Result code: " +
				responseCode + ". Response: " + TranslateServerResponseCode(responseCode));
			nativePurchaseComplete(UndefinedFailureResponse, "", "", "", "");
		}
	}


	///////////////////////////////////////////////////////
	// Internal helper functions that deal assist with various IAB related events

	/**
	 * Get a text tranlation of the Response Codes returned by google play.
	 */
	private String TranslateServerResponseCode(final int serverResponseCode)
	{
		// taken from https://developer.android.com/reference/com/android/billingclient/api/BillingClient.BillingResponseCode
		switch(serverResponseCode)
		{
			case BillingClient.BillingResponseCode.OK:
				return "Success";
			case BillingClient.BillingResponseCode.USER_CANCELED:
				return "User pressed back or canceled a dialog";
			case BillingClient.BillingResponseCode.SERVICE_UNAVAILABLE:
				return "Network connection is down or there was a timeout";
			case BillingClient.BillingResponseCode.SERVICE_DISCONNECTED:
				return "Play Store service is not connected now - potentially transient state";
			case BillingClient.BillingResponseCode.BILLING_UNAVAILABLE:
				return "Billing API version is not supported for the type requested";
			case BillingClient.BillingResponseCode.FEATURE_NOT_SUPPORTED:
				return "Requested feature is not supported by Play Store on the current device";
			case BillingClient.BillingResponseCode.ITEM_UNAVAILABLE:
				return "Requested product is not available for purchase";
			case BillingClient.BillingResponseCode.DEVELOPER_ERROR:
				return "Invalid arguments provided to the API. This error can also indicate that the application was not correctly signed or properly set up for In-app Billing in Google Play, or does not have the necessary permissions in its manifest";
			case BillingClient.BillingResponseCode.ERROR:
				return "Fatal error during the API action";
			case BillingClient.BillingResponseCode.ITEM_ALREADY_OWNED:
				return "Failure to purchase since item is already owned";
			case BillingClient.BillingResponseCode.ITEM_NOT_OWNED:
				return "Failure to consume since item is not owned";
			default:
				return "Unknown Server Response Code";
		}
	}

	// Callback that notify the C++ implementation that a task has completed
	public native void nativeQueryComplete(int responseCode, String[] productIDs, String[] titles, String[] descriptions, String[] prices, float[] pricesRaw, String[] currencyCodes);
	public native void nativePurchaseComplete(int responseCode, String ProductId, String ProductToken, String ReceiptData, String Signature);
	public native void nativeRestorePurchasesComplete(int responseCode, String[] ProductIds, String[] ProductTokens, String[] ReceiptsData, String[] Signatures);
	public native void nativeQueryExistingPurchasesComplete(int responseCode, String[] ProductIds, String[] ProductTokens, String[] ReceiptsData, String[] Signatures);
}

