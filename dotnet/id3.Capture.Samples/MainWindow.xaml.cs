using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace id3.Capture.Samples
{
    using id3.Camera;

    /// <summary>
    /// Logique d'interaction pour MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        // simple lock for controls 
        volatile int _lockControls;
        // current status
        string _status;
        // camera slot
        volatile int _cameraSlot = 0;
        // current live image
        id3.Image.Image _currentPicture;
        // current live bitmap
        WriteableBitmap _currentBitmap;

        public MainWindow()
        {
            InitializeComponent();
            DataContext = this;
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            _lockControls++;
            comboBoxCameraList.Items.Add("<NO CAMERA SELECTED>");
            comboBoxCameraList.SelectedIndex = 0;
            _lockControls--;

            _currentPicture = new Image.Image();

            // id3Camera initialisation
            try
            {
                CameraManager.Initialize();

                CameraManager.PlugAndPlayEvent += CameraManager_PlugAndPlayEvent;
                CameraManager.CameraDeviceEvent += CameraManager_CameraDeviceEvent;
                CameraManager.CameraSelectedEvent += CameraManager_CameraSelectedEvent;
                CameraManager.ImageCapturedEvent += CameraManager_ImageCaptureEvent;

                CameraManager.LoadPlugin("id3CameraWebcam");
                CameraManager.EnableCameraModel("Webcam", true);

                CameraManager.Start();
                CameraManager.StartPlugAndPlay();
            }
            catch (Exception ex)
            {
                MessageBox.Show(string.Format("CaptureManager exception: {0}", ex.Message));
                Environment.Exit(-1);
            }

            Status = "Initialization done. camera plug & play process started.";
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            // Stop capture process
            CameraManager.Stop();
            _currentPicture.Dispose();
        }

        #region Camera events
        private void CameraManager_CameraAdded(object sender, EventArgs e)
        {
            var camera = (CameraInfo)sender;
            if (camera != null)
            {
                Status = "Found camera " + camera.Name;

                _lockControls++;
                int cam_idx = comboBoxCameraList.Items.Add(camera);
                _lockControls--;

                if (comboBoxCameraList.SelectedIndex == 0)
                {
                    if (comboBoxCameraList.Items.Count > 0)
                    {
                        comboBoxCameraList.SelectedIndex = 1;
                    }
                }
            }
        }

        private void CameraManager_CameraRemoved(object sender, EventArgs e)
        {
            var camera = (CameraInfo)sender;
            if (camera != null)
            {
                Status = "Lost camera " + camera.Name;

                _lockControls++;
                comboBoxCameraList.Items.Remove(camera);
                _lockControls--;
            }
        }

        private void CameraManager_PlugAndPlayEvent(object sender, PlugAndPlayEventArgs e)
        {
            switch (e.Type)
            {
                case id3CameraPlugAndPlayEventType.CameraAdded: CameraManager_CameraAdded(sender, e); break;
                case id3CameraPlugAndPlayEventType.CameraRemoved: CameraManager_CameraRemoved(sender, e); break;
            }
        }

        private void CameraManager_CameraDeviceEvent(object sender, CameraDeviceEventArgs e)
        {
            var camera = (Camera)sender;
            if (e.Type == id3CameraDeviceEventType.DeviceReady)
            {
                int camera_id = CameraManager.GetSelectedCamera(_cameraSlot);
                if (camera_id == e.Device_id)
                {
                    CameraManager.StartCamera(e.Device_id);
                }
            }
            else if (e.Type == id3CameraDeviceEventType.DeviceError)
            {
                string errmsg = camera.GetLastError();
                Status = String.Format("camera last error : {0}", errmsg);
            }
        }

        private void CameraManager_CameraSelectedEvent(object sender, CameraSelectedEventArgs e)
        {
            // Destroy to force refresh
            _currentBitmap = null;

            var camera = (Camera)sender;
            if ((e.Device_id != 0) && (e.CameraSlot == _cameraSlot))
            {
                // Restore selection because of unselect event !
                _lockControls++;
                comboBoxCameraList.SelectedIndex = (int)comboBoxCameraList.Tag;
                _lockControls--;

                // auto select 720p
                CameraResolution auto_select = camera.FindNearestResolution(1280, 720, 30);
                if (auto_select == null)
                {
                    // auto select low res 480p
                    auto_select = camera.FindNearestResolution(640, 480, 20);
                }
                if (auto_select != null)
                {
                    camera.SetParameter("Resolution", auto_select.Value);
                }
            }
            else
            {
                _lockControls++;
                comboBoxCameraList.SelectedIndex = 0;
                _lockControls--;
            }
        }

        private void CameraManager_ImageCaptureEvent(object sender, ImageCapturedEventArgs e)
        {
            // Get current camera for our camera slot and process only if good camera and page is visible
            int camera_id = CameraManager.GetSelectedCamera(_cameraSlot);
            if ((e.Device_id == camera_id) && (IsVisible))
            {
                var measurement_data = new CameraMeasurementData();
                var camera = new Camera(camera_id);
                camera.GetMeasurementData(measurement_data);
                var cam_fps = measurement_data.GetDataFloat("AcquireFrameRate");
                var cam_ctr = measurement_data.GetDataInt("AcquireFrameCount");
                string fps_str = $"F ={cam_fps:00.00} fps {cam_ctr:00000000}";
                Status = fps_str;

                // Get current camera frame
                int err = CameraManager.GetCurrentFrame(camera_id, _currentPicture);
                if ((err == 0) && (_currentPicture.Height > 0))
                {
                    ToBitmapSource(_currentPicture, ref _currentBitmap);

                    // Draw current frame
                    imagePreview.BeginInit();
                    imagePreview.Source = _currentBitmap;
                    imagePreview.EndInit();
                }
            }
        }
        #endregion

        string Status
        {
            get { return _status; }
            set
            {
                if (_status != value)
                {
                    _status = value;
                    lblStatus.Text = value;
                }
            }
        }

        private void CameraList_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (_lockControls > 0)
                return;

            if (comboBoxCameraList.SelectedItem != null)
            {
                comboBoxCameraList.Tag = comboBoxCameraList.SelectedIndex;
                if (comboBoxCameraList.SelectedItem.GetType() == typeof(CameraInfo))
                {
                    var cameraInfo = comboBoxCameraList.SelectedItem as CameraInfo;
                    CameraManager.SelectCamera(_cameraSlot, cameraInfo.Device_id);
                }
                else
                {
                    CameraManager.SelectCamera(_cameraSlot, 0);
                }
            }
        }

        public static BitmapSource ToBitmapSource(id3.Image.Image image, ref WriteableBitmap wbm)
        {
            if (image.PixelDepth != 1)
            {
                int w = image.Width;
                int h = image.Height;
                int bufferSize = image.Height * image.Stride;

                if (wbm == null)
                    wbm = new WriteableBitmap(w, h, 96, 96, PixelFormats.Bgr24, null);

                if ((h <= wbm.Height) && (w <= wbm.Width))
                {
                    wbm.WritePixels(new Int32Rect(0, 0, w, h), image.PixelsPtr, bufferSize, image.Stride);
                }
            }
            return wbm;
        }
    }
}
