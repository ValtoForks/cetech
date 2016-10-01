using System;
using CETech.CEMath;

namespace CETech
{
    /// <summary>
    ///     Window
    /// </summary>
    public partial class Window
    {
        private string _title;

        /// <summary>
        ///     Create new window
        /// </summary>
        /// <param name="title">Window title</param>
        /// <param name="x">Create X position</param>
        /// <param name="y">Create Y position</param>
        /// <param name="width">Width</param>
        /// <param name="height">Height</param>
        /// <param name="flags">Flags</param>
        public Window(string title, WindowPos x, WindowPos y, int width, int height, int flags)
        {
            Width = width;
            Height = height;

            CtorImpl(title, x, y, width, height, flags);
        }

        /// <summary>
        ///     Create new window from native window pointer.
        /// </summary>
        /// <param name="id">Native window pointer.</param>
        public Window(IntPtr id)
        {
            CtorImpl(id);
        }

        /// <summary>
        ///     Window title
        /// </summary>
        public string Title
        {
            get { return _title; }
            set
            {
                _title = value;
                SetTitleImpl(value);
            }
        }

        /// <summary>
        ///     Native window ptr
        /// </summary>
        public IntPtr NativeWindowPtr
        {
            get { return GetNativePtrImpl(); }
        }

        /// <summary>
        ///     Native display ptr
        /// </summary>
        public IntPtr NativeDisplayPtr
        {
            get { return GetNativeDisplayPtrImpl(); }
        }

        /// <summary>
        ///     Window height
        /// </summary>
        public int Height { get; private set; }

        /// <summary>
        ///     Widnow width
        /// </summary>
        public int Width { get; private set; }

        /// <summary>
        ///     Update window surface
        /// </summary>
        public void Update()
        {
            UpdateImpl();
        }

        /// <summary>
        ///     Resize window
        /// </summary>
        /// <param name="width">Window</param>
        /// <param name="height">Height</param>
        public void Resize(int width, int height)
        {
            ResizeImpl(width, height);
            Width = width;
            Height = height;
        }

        public void SetCursorPosition(Vec2f pos)
        {
            SetCursorPositionImpl(pos);
        }

        /// <summary>
        ///     Window destructor
        /// </summary>
        ~Window()
        {
            DtorImpl();
        }
    }
}