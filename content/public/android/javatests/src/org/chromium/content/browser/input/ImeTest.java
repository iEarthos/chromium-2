// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.input;

import android.app.Activity;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.test.suitebuilder.annotation.MediumTest;
import android.test.suitebuilder.annotation.SmallTest;
import android.text.TextUtils;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.EditorInfo;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.UrlUtils;
import org.chromium.content.browser.ContentView;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content.browser.test.util.DOMUtils;
import org.chromium.content.browser.test.util.TestCallbackHelperContainer;
import org.chromium.content.browser.test.util.TestInputMethodManagerWrapper;
import org.chromium.content_shell_apk.ContentShellTestBase;

import java.util.ArrayList;
import java.util.concurrent.Callable;

/**
 * Integration tests for text input using cases based on fixed regressions.
 */
public class ImeTest extends ContentShellTestBase {

    private static final String DATA_URL = UrlUtils.encodeHtmlDataUri(
            "<html><head><meta name=\"viewport\"" +
            "content=\"width=device-width, initial-scale=2.0, maximum-scale=2.0\" /></head>" +
            "<body><form action=\"about:blank\">" +
            "<input id=\"input_text\" type=\"text\" /><br/>" +
            "<input id=\"input_radio\" type=\"radio\" style=\"width:50px;height:50px\" />" +
            "<br/><textarea id=\"textarea\" rows=\"4\" cols=\"20\"></textarea>" +
            "</form></body></html>");

    private TestAdapterInputConnection mConnection;
    private ImeAdapter mImeAdapter;
    private ContentView mContentView;
    private TestCallbackHelperContainer mCallbackContainer;
    private TestInputMethodManagerWrapper mInputMethodManagerWrapper;

    @Override
    public void setUp() throws Exception {
        super.setUp();

        launchContentShellWithUrl(DATA_URL);
        assertTrue("Page failed to load", waitForActiveShellToBeDoneLoading());

        mInputMethodManagerWrapper = new TestInputMethodManagerWrapper(getContentViewCore());
        getImeAdapter().setInputMethodManagerWrapper(mInputMethodManagerWrapper);
        assertEquals(0, mInputMethodManagerWrapper.getShowSoftInputCounter());
        getContentViewCore().setAdapterInputConnectionFactory(
                new TestAdapterInputConnectionFactory());

        mContentView = getActivity().getActiveContentView();
        mCallbackContainer = new TestCallbackHelperContainer(mContentView);
        // TODO(aurimas) remove this wait once crbug.com/179511 is fixed.
        assertWaitForPageScaleFactorMatch(2);
        assertTrue(
                DOMUtils.waitForNonZeroNodeBounds(mContentView, mCallbackContainer, "input_text"));
        DOMUtils.clickNode(this, mContentView, mCallbackContainer, "input_text");
        assertWaitForKeyboardStatus(true);

        mConnection = (TestAdapterInputConnection) getAdapterInputConnection();
        mImeAdapter = getImeAdapter();

        waitAndVerifyEditableCallback(mConnection.mImeUpdateQueue, 0, "", 0, 0, -1, -1);
        assertEquals(1, mInputMethodManagerWrapper.getShowSoftInputCounter());
        assertEquals(0, mInputMethodManagerWrapper.getEditorInfo().initialSelStart);
        assertEquals(0, mInputMethodManagerWrapper.getEditorInfo().initialSelEnd);
    }

    @MediumTest
    @Feature({"TextInput", "Main"})
    public void testKeyboardDismissedAfterClickingGo() throws Throwable {
        setComposingText(mConnection, "hello", 1);
        waitAndVerifyEditableCallback(mConnection.mImeUpdateQueue, 1, "hello", 5, 5, 0, 5);

        performGo(getAdapterInputConnection(), mCallbackContainer);

        waitAndVerifyEditableCallback(mConnection.mImeUpdateQueue, 2, "", 0, 0, -1, -1);
        assertWaitForKeyboardStatus(false);
    }

    @SmallTest
    @Feature({"TextInput", "Main"})
    public void testGetTextUpdatesAfterEnteringText() throws Throwable {
        setComposingText(mConnection, "h", 1);
        waitAndVerifyEditableCallback(mConnection.mImeUpdateQueue, 1, "h", 1, 1, 0, 1);
        assertEquals(1, mInputMethodManagerWrapper.getShowSoftInputCounter());

        setComposingText(mConnection, "he", 1);
        waitAndVerifyEditableCallback(mConnection.mImeUpdateQueue, 2, "he", 2, 2, 0, 2);
        assertEquals(1, mInputMethodManagerWrapper.getShowSoftInputCounter());

        setComposingText(mConnection, "hel", 1);
        waitAndVerifyEditableCallback(mConnection.mImeUpdateQueue, 3, "hel", 3, 3, 0, 3);
        assertEquals(1, mInputMethodManagerWrapper.getShowSoftInputCounter());

        commitText(mConnection, "hel", 1);
        waitAndVerifyEditableCallback(mConnection.mImeUpdateQueue, 4, "hel", 3, 3, -1, -1);
        assertEquals(1, mInputMethodManagerWrapper.getShowSoftInputCounter());
    }

    @SmallTest
    @Feature({"TextInput"})
    public void testImeCopy() throws Exception {
        commitText(mConnection, "hello", 1);
        waitAndVerifyEditableCallback(mConnection.mImeUpdateQueue, 1, "hello", 5, 5, -1, -1);

        setSelection(mConnection, 2, 5);
        waitAndVerifyEditableCallback(mConnection.mImeUpdateQueue, 2, "hello", 2, 5, -1, -1);

        copy(mImeAdapter);
        assertClipboardContents(getActivity(), "llo");
    }

    @SmallTest
    @Feature({"TextInput"})
    public void testEnterTextAndRefocus() throws Exception {
        commitText(mConnection, "hello", 1);
        waitAndVerifyEditableCallback(mConnection.mImeUpdateQueue, 1, "hello", 5, 5, -1, -1);

        DOMUtils.clickNode(this, mContentView, mCallbackContainer, "input_radio");
        assertWaitForKeyboardStatus(false);

        DOMUtils.clickNode(this, mContentView, mCallbackContainer, "input_text");
        assertWaitForKeyboardStatus(true);
        assertEquals(5, mInputMethodManagerWrapper.getEditorInfo().initialSelStart);
        assertEquals(5, mInputMethodManagerWrapper.getEditorInfo().initialSelEnd);
    }

    @SmallTest
    @Feature({"TextInput"})
    public void testImeCut() throws Exception {
        commitText(mConnection, "snarful", 1);
        waitAndVerifyEditableCallback(mConnection.mImeUpdateQueue, 1, "snarful", 7, 7, -1, -1);

        setSelection(mConnection, 1, 5);
        waitAndVerifyEditableCallback(mConnection.mImeUpdateQueue, 2, "snarful", 1, 5, -1, -1);

        cut(mImeAdapter);
        waitAndVerifyEditableCallback(mConnection.mImeUpdateQueue, 3, "sul", 1, 1, -1, -1);

        assertClipboardContents(getActivity(), "narf");
    }

    @SmallTest
    @Feature({"TextInput"})
    public void testImePaste() throws Exception {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                ClipboardManager clipboardManager =
                        (ClipboardManager) getActivity().getSystemService(
                                Context.CLIPBOARD_SERVICE);
                clipboardManager.setPrimaryClip(ClipData.newPlainText("blarg", "blarg"));
            }
        });

        paste(mImeAdapter);
        waitAndVerifyEditableCallback(mConnection.mImeUpdateQueue, 1, "blarg", 5, 5, -1, -1);

        setSelection(mConnection, 3, 5);
        waitAndVerifyEditableCallback(mConnection.mImeUpdateQueue, 2, "blarg", 3, 5, -1, -1);

        paste(mImeAdapter);
        // Paste is a two step process when there is a non-zero selection.
        waitAndVerifyEditableCallback(mConnection.mImeUpdateQueue, 3, "bla", 3, 3, -1, -1);
        waitAndVerifyEditableCallback(mConnection.mImeUpdateQueue, 4, "blablarg", 8, 8, -1, -1);

        paste(mImeAdapter);
        waitAndVerifyEditableCallback(
                mConnection.mImeUpdateQueue, 5, "blablargblarg", 13, 13, -1, -1);
    }

    @SmallTest
    @Feature({"TextInput"})
    public void testImeSelectAndUnSelectAll() throws Exception {
        commitText(mConnection, "hello", 1);
        waitAndVerifyEditableCallback(mConnection.mImeUpdateQueue, 1, "hello", 5, 5, -1, -1);

        selectAll(mImeAdapter);
        waitAndVerifyEditableCallback(mConnection.mImeUpdateQueue, 2, "hello", 0, 5, -1, -1);

        unselect(mImeAdapter);
        waitAndVerifyEditableCallback(mConnection.mImeUpdateQueue, 3, "", 0, 0, -1, -1);

        assertWaitForKeyboardStatus(false);
    }

    @SmallTest
    @Feature({"TextInput", "Main"})
    public void testShowImeIfNeeded() throws Throwable {
        DOMUtils.focusNode(this, mContentView, mCallbackContainer, "input_radio");
        assertWaitForKeyboardStatus(false);

        performShowImeIfNeeded();
        assertWaitForKeyboardStatus(false);

        DOMUtils.focusNode(this, mContentView, mCallbackContainer, "input_text");
        assertWaitForKeyboardStatus(false);

        performShowImeIfNeeded();
        assertWaitForKeyboardStatus(true);
    }

    @SmallTest
    @Feature({"TextInput", "Main"})
    public void testFinishComposingText() throws Throwable {
        // Focus the textarea. We need to do the following steps because we are focusing using JS.
        DOMUtils.focusNode(this, mContentView, mCallbackContainer, "input_radio");
        assertWaitForKeyboardStatus(false);
        DOMUtils.focusNode(this, mContentView, mCallbackContainer, "textarea");
        assertWaitForKeyboardStatus(false);
        performShowImeIfNeeded();
        assertWaitForKeyboardStatus(true);

        mConnection = (TestAdapterInputConnection) getAdapterInputConnection();
        waitAndVerifyEditableCallback(mConnection.mImeUpdateQueue, 0, "", 0, 0, -1, -1);

        commitText(mConnection, "hllo", 1);
        waitAndVerifyEditableCallback(mConnection.mImeUpdateQueue, 1, "hllo", 4, 4, -1, -1);

        commitText(mConnection, " ", 1);
        waitAndVerifyEditableCallback(mConnection.mImeUpdateQueue, 2, "hllo ", 5, 5, -1, -1);

        setSelection(mConnection, 1, 1);
        waitAndVerifyEditableCallback(mConnection.mImeUpdateQueue, 3, "hllo ", 1, 1, -1, -1);

        setComposingRegion(mConnection, 0, 4);
        waitAndVerifyEditableCallback(mConnection.mImeUpdateQueue, 4, "hllo ", 1, 1, 0, 4);

        finishComposingText(mConnection);
        waitAndVerifyEditableCallback(mConnection.mImeUpdateQueue, 5, "hllo ", 1, 1, -1, -1);

        commitText(mConnection, "\n", 1);
        waitAndVerifyEditableCallback(mConnection.mImeUpdateQueue, 6, "h\nllo ", 2, 2, -1, -1);
    }

    @SmallTest
    @Feature({"TextInput", "Main"})
    public void testEnterKeyEventWhileComposingText() throws Throwable {
        // Focus the textarea. We need to do the following steps because we are focusing using JS.
        DOMUtils.focusNode(this, mContentView, mCallbackContainer, "input_radio");
        assertWaitForKeyboardStatus(false);
        DOMUtils.focusNode(this, mContentView, mCallbackContainer, "textarea");
        assertWaitForKeyboardStatus(false);
        performShowImeIfNeeded();
        assertWaitForKeyboardStatus(true);

        mConnection = (TestAdapterInputConnection) getAdapterInputConnection();
        waitAndVerifyEditableCallback(mConnection.mImeUpdateQueue, 0, "", 0, 0, -1, -1);

        setComposingText(mConnection, "hello", 1);
        waitAndVerifyEditableCallback(mConnection.mImeUpdateQueue, 1, "hello", 5, 5, 0, 5);

        getInstrumentation().runOnMainSync(new Runnable() {
            @Override
            public void run() {
                mConnection.sendKeyEvent(
                        new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_ENTER));
                mConnection.sendKeyEvent(new KeyEvent(KeyEvent.ACTION_UP, KeyEvent.KEYCODE_ENTER));
            }
        });

        // TODO(aurimas): remove this workaround when crbug.com/278584 is fixed.
        waitAndVerifyEditableCallback(mConnection.mImeUpdateQueue, 2, "hello", 5, 5, -1, -1);
        // The second new line is not a user visible/editable one, it is a side-effect of Blink
        // using <br> internally.
        waitAndVerifyEditableCallback(mConnection.mImeUpdateQueue, 3, "hello\n\n", 6, 6, -1, -1);
    }

    private void performShowImeIfNeeded() {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mContentView.getContentViewCore().showImeIfNeeded();
            }
        });
    }

    private void performGo(final AdapterInputConnection inputConnection,
            TestCallbackHelperContainer testCallbackHelperContainer) throws Throwable {
        handleBlockingCallbackAction(
                testCallbackHelperContainer.getOnPageFinishedHelper(),
                new Runnable() {
                    @Override
                    public void run() {
                        inputConnection.performEditorAction(EditorInfo.IME_ACTION_GO);
                    }
                });
    }

    private void assertWaitForKeyboardStatus(final boolean show) throws InterruptedException {
        assertTrue(CriteriaHelper.pollForCriteria(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return show == getImeAdapter().mIsShowWithoutHideOutstanding &&
                        (!show || getAdapterInputConnection() != null);
            }
        }));
    }

    private void waitAndVerifyEditableCallback(final ArrayList<TestImeState> states,
            final int index, String text, int selectionStart, int selectionEnd,
            int compositionStart, int compositionEnd) throws InterruptedException {
        assertTrue(CriteriaHelper.pollForCriteria(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return states.size() > index;
            }
        }));
        states.get(index).assertEqualState(
                text, selectionStart, selectionEnd, compositionStart, compositionEnd);
    }

    private void assertClipboardContents(final Activity activity, final String expectedContents)
            throws InterruptedException {
        assertTrue(CriteriaHelper.pollForCriteria(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return ThreadUtils.runOnUiThreadBlockingNoException(new Callable<Boolean>() {
                    @Override
                    public Boolean call() throws Exception {
                        ClipboardManager clipboardManager =
                                (ClipboardManager) activity.getSystemService(
                                        Context.CLIPBOARD_SERVICE);
                        ClipData clip = clipboardManager.getPrimaryClip();
                        return clip != null && clip.getItemCount() == 1
                                && TextUtils.equals(clip.getItemAt(0).getText(), expectedContents);
                    }
                });
            }
        }));
    }

    private ImeAdapter getImeAdapter() {
        return getContentViewCore().getImeAdapterForTest();
    }

    private AdapterInputConnection getAdapterInputConnection() {
        return getContentViewCore().getInputConnectionForTest();
    }

    private void copy(final ImeAdapter adapter) {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                adapter.copy();
            }
        });
    }

    private void cut(final ImeAdapter adapter) {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                adapter.cut();
            }
        });
    }

    private void paste(final ImeAdapter adapter) {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                adapter.paste();
            }
        });
    }

    private void selectAll(final ImeAdapter adapter) {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                adapter.selectAll();
            }
        });
    }

    private void unselect(final ImeAdapter adapter) {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                adapter.unselect();
            }
        });
    }

    private void commitText(final AdapterInputConnection connection, final CharSequence text,
            final int newCursorPosition) {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                connection.commitText(text, newCursorPosition);
            }
        });
    }

    private void setSelection(final AdapterInputConnection connection, final int start,
            final int end) {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                connection.setSelection(start, end);
            }
        });
    }

    private void setComposingRegion(final AdapterInputConnection connection, final int start,
            final int end) {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                connection.setComposingRegion(start, end);
            }
        });
    }

    private void setComposingText(final AdapterInputConnection connection, final CharSequence text,
            final int newCursorPosition) {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                connection.setComposingText(text, newCursorPosition);
            }
        });
    }

    private void finishComposingText(final AdapterInputConnection connection) {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                connection.finishComposingText();
            }
        });
    }

    private static class TestAdapterInputConnectionFactory extends
            ImeAdapter.AdapterInputConnectionFactory {
        @Override
        public AdapterInputConnection get(View view, ImeAdapter imeAdapter,
                EditorInfo outAttrs) {
            return new TestAdapterInputConnection(view, imeAdapter, outAttrs);
        }
    }

    private static class TestAdapterInputConnection extends AdapterInputConnection {
        private final ArrayList<TestImeState> mImeUpdateQueue = new ArrayList<TestImeState>();

        public TestAdapterInputConnection(View view, ImeAdapter imeAdapter, EditorInfo outAttrs) {
            super(view, imeAdapter, outAttrs);
        }

        @Override
        public void updateState(String text, int selectionStart, int selectionEnd,
                int compositionStart, int compositionEnd, boolean requiredAck) {
            mImeUpdateQueue.add(new TestImeState(text, selectionStart, selectionEnd,
                    compositionStart, compositionEnd));
            super.updateState(text, selectionStart, selectionEnd, compositionStart,
                    compositionEnd, requiredAck);
        }
    }

    private static class TestImeState {
        private final String mText;
        private final int mSelectionStart;
        private final int mSelectionEnd;
        private final int mCompositionStart;
        private final int mCompositionEnd;

        public TestImeState(String text, int selectionStart, int selectionEnd,
                int compositionStart, int compositionEnd) {
            mText = text;
            mSelectionStart = selectionStart;
            mSelectionEnd = selectionEnd;
            mCompositionStart = compositionStart;
            mCompositionEnd = compositionEnd;
        }

        public void assertEqualState(String text, int selectionStart, int selectionEnd,
                int compositionStart, int compositionEnd) {
            assertEquals("Text did not match", text, mText);
            assertEquals("Selection start did not match", selectionStart, mSelectionStart);
            assertEquals("Selection end did not match", selectionEnd, mSelectionEnd);
            assertEquals("Composition start did not match", compositionStart, mCompositionStart);
            assertEquals("Composition end did not match", compositionEnd, mCompositionEnd);
        }
    }
}
