import javax.swing.*;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.InputEvent;
import java.awt.event.KeyEvent;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileWriter;
import java.io.IOException;
import java.util.LinkedList;
import java.util.Scanner;

public class MyEditor {

    static boolean actionsSetup = false;
    static int helpTabIdx = -1;
    static Action paste;
    static Action copy;
    static Action cut;
    static Action selectAll;
    static Action delete;

    LinkedList<MyTab> tabsList;
    private JFrame frame;
    private JPanel mainPanel;
    private JTabbedPane tabsPanel;
    private JMenuBar menu;

    class MyTab {
        private File f;
        protected JTextArea jta;
        protected JScrollPane jsp;
        protected TabButton tabComponent;

        MyTab( File file ){

            f = file;
            jta = new JTextArea();
            JPopupMenu popup = new JPopupMenu();

            if( !actionsSetup )
                setupEditActions();
            popup.add(new JMenuItem(cut));
            popup.add(new JMenuItem(copy));
            popup.add(new JMenuItem(paste));
            popup.add(new JMenuItem(selectAll));
            popup.add(new JMenuItem(delete));
            jta.setComponentPopupMenu(popup);

            tabComponent = new TabButton( f.getName() );

            jsp = new JScrollPane( jta );
        }

        class TabButton extends JPanel{
            private JLabel tabTitle;
            private JButton closeButton;
            TabButton( String title ){
                tabTitle = new JLabel( title );
                this.setOpaque( false );
                closeButton = new JButton();
                closeButton.setMargin(new Insets(0, 0, 2, 0));
                closeButton.setToolTipText( "Closes this tab");
                closeButton.setPreferredSize( new Dimension( 15, 15 ));
                closeButton.setText( "x" );
                this.add( tabTitle );
                this.add( closeButton );
            }
            void changeTitle( String newTitle ){
                tabTitle.setText( newTitle );
            }
        }

        void changeFileAndTitle( File newFile ){
            f = newFile;
            tabComponent.changeTitle( f.getName() );
        }
        TabButton getTabComponent(){ return tabComponent; }
        JTextArea getTextArea(){ return jta; }
        JScrollPane getScrollPane(){
            return jsp;
        }
        File getFile(){ return f; }
    }

    class HelpTab extends MyTab{

        HelpTab(){
            super( new File("Help") );
            jta.setText( "Use Ctrl+O or go to File -> Open to open a file.\n");
            jta.setEnabled( false );
        }
    }

    class SaveAsHandler implements ActionListener{

        SaveAsHandler(){}

        @Override
        public void actionPerformed(ActionEvent ae) {

            int activeTabIdx = tabsPanel.getSelectedIndex();
            if( activeTabIdx == helpTabIdx )
                return;
            JFileChooser fc = new JFileChooser();
            int retVal = fc.showSaveDialog(frame);

            if (retVal == JFileChooser.APPROVE_OPTION) {
                File selectedFile = fc.getSelectedFile();
                if ( selectedFile.exists() ) {
                    int option = JOptionPane.showConfirmDialog( fc, "The file " + selectedFile.getName() + " already exists.\nDo you want to overwrite it?",
                    "Overwrite confirmation", JOptionPane.YES_NO_OPTION, JOptionPane.WARNING_MESSAGE );
                    if ( option == JOptionPane.NO_OPTION ) {
                        return;
                    }
                }
                try {
                    if (!selectedFile.exists() && !selectedFile.createNewFile() ) {// if doesnt exist and failed to create new file
                        JOptionPane.showMessageDialog(fc,"Could not create file.");
                        return;
                    }
                }
                catch( IOException ex ){
                    JOptionPane.showMessageDialog(fc,"IO Error: could not save to file.");
                    return;
                }

                try (FileWriter fileWriter = new FileWriter(selectedFile, false)) {
                    fileWriter.write(tabsList.get(activeTabIdx).getTextArea().getText());
                    tabsList.get(activeTabIdx).changeFileAndTitle( selectedFile );

                } catch (IOException ex) {
                    JOptionPane.showMessageDialog(fc,"IO Error: could not save to file.");
                    return;
                }
            }
        }
    }

    private void addTab( MyTab tab ){
        if( tab instanceof HelpTab )
            helpTabIdx = tabsList.size();
        tabsList.addLast( tab );
        tabsPanel.add( tab.getScrollPane() );
        tab.getTabComponent().closeButton.addActionListener( e ->{
            removeTab( tabsPanel.getSelectedIndex() );
        } );
        tabsPanel.setTabComponentAt( tabsList.size()-1, tab.getTabComponent() );
    }
    private void removeTab( int idx ){
        if( idx == helpTabIdx )
            helpTabIdx = -1;
        tabsPanel.remove( idx );
        tabsList.remove( idx );
    }

    private void setupEditActions(){

        JTextArea jta = new JTextArea();

        ActionMap am = jta.getActionMap();

        paste = am.get("paste-from-clipboard");
        copy = am.get("copy-to-clipboard");
        cut = am.get("cut-to-clipboard");
        selectAll = am.get("select-all");
        delete = am.get("delete-previous");

        cut.putValue(Action.NAME, "Cut");
        copy.putValue(Action.NAME, "Copy");
        paste.putValue(Action.NAME, "Paste");
        selectAll.putValue(Action.NAME, "Select all");
        delete.putValue(Action.NAME, "Delete");

        actionsSetup = true;
    }

    private JMenu setupFileSubmenu(){

        JMenu fileMenu = new JMenu("File");

        JMenuItem open = new JMenuItem("Open");
        open.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_O, InputEvent.CTRL_DOWN_MASK));
        open.addActionListener(e -> {

            JFileChooser fc = new JFileChooser();
            int retVal = fc.showOpenDialog(frame);

            if (retVal == JFileChooser.APPROVE_OPTION) {
                File selectedFile = fc.getSelectedFile();
                try {
                    if (!selectedFile.exists() && !selectedFile.createNewFile() ) {// if doesnt exist and failed to create new file
                        // handling
                        return;
                    }
                }
                catch( IOException ex ){
                    // handling
                    return;
                }

                try (Scanner fileReader = new Scanner(selectedFile)) {

                    addTab( new MyTab( selectedFile ) );

                    tabsPanel.setSelectedIndex(tabsPanel.getTabCount() - 1);

                    while (fileReader.hasNextLine()) {
                        tabsList.getLast().getTextArea().append(fileReader.nextLine() + "\n");
                    }
                } catch (FileNotFoundException ex) {
                    // handling
                    return;
                }
            }
        });

        JMenuItem save = new JMenuItem("Save");
        save.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_S, InputEvent.CTRL_DOWN_MASK));
        save.addActionListener(e -> {

            int activeTabIdx = tabsPanel.getSelectedIndex();
            if( activeTabIdx == helpTabIdx )
                return;
            File selectedFile = tabsList.get(activeTabIdx).getFile();
            if( !selectedFile.exists() ){
                new SaveAsHandler().actionPerformed( e );
                return;
            }

            try (FileWriter fileWriter = new FileWriter(selectedFile, false)) {
                fileWriter.write(tabsList.get(activeTabIdx).getTextArea().getText());
            } catch (IOException ex) {
                JOptionPane.showMessageDialog( tabsPanel,"IO Error: could not save to file.");
                return;
            }
        });

        JMenuItem saveAs = new JMenuItem("Save As ...");
        saveAs.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_F12, 0));
        saveAs.addActionListener( new SaveAsHandler() );

        JMenuItem exit = new JMenuItem("Exit");
        exit.addActionListener( e -> {
            frame.dispose();
            System.exit(0);
        });

        fileMenu.add(open);
        fileMenu.add(save);
        fileMenu.add(saveAs);
        fileMenu.add(exit);

        return fileMenu;
    }

    private JMenu setupEditSubmenu(){

        JMenu editMenu = new JMenu("Edit");
        if( !actionsSetup )
            setupEditActions();
        JMenuItem cutItem = new JMenuItem(cut);
        JMenuItem copyItem = new JMenuItem(copy);
        JMenuItem pasteItem = new JMenuItem(paste);
        JMenuItem deleteItem = new JMenuItem(delete);
        JMenuItem selectAllItem = new JMenuItem(selectAll);
        cutItem.setAccelerator(KeyStroke.getKeyStroke( KeyEvent.VK_X, InputEvent.CTRL_DOWN_MASK ));
        copyItem.setAccelerator(KeyStroke.getKeyStroke( KeyEvent.VK_C, InputEvent.CTRL_DOWN_MASK ));
        pasteItem.setAccelerator(KeyStroke.getKeyStroke( KeyEvent.VK_V, InputEvent.CTRL_DOWN_MASK ));
        deleteItem.setAccelerator(KeyStroke.getKeyStroke( KeyEvent.VK_DELETE, 0 ));
        selectAllItem.setAccelerator(KeyStroke.getKeyStroke( KeyEvent.VK_A, InputEvent.CTRL_DOWN_MASK ));
        editMenu.add( cutItem );
        editMenu.add( copyItem );
        editMenu.add( pasteItem );
        editMenu.add( deleteItem );
        editMenu.add( selectAllItem );

        return editMenu;
    }

    private JMenu setupFormatSubmenu(){

        JMenu format = new JMenu( "Format" );

        JMenuItem increaseSize = new JMenuItem( "Increase font size" );
        increaseSize.setAccelerator(KeyStroke.getKeyStroke( KeyEvent.VK_ADD, InputEvent.CTRL_DOWN_MASK ) );
        increaseSize.addActionListener( e -> {
            int activeTabIdx = tabsPanel.getSelectedIndex();
            JTextArea currTextArea = tabsList.get( activeTabIdx ).getTextArea();
            Font oldFont = currTextArea.getFont();
            Font newFont = new Font( oldFont.getName(), oldFont.getStyle(), oldFont.getSize() + 1 );
            currTextArea.setFont( newFont );
        });
        JMenuItem decreaseSize = new JMenuItem( "Decrease font size" );
        decreaseSize.setAccelerator(KeyStroke.getKeyStroke( KeyEvent.VK_SUBTRACT, InputEvent.CTRL_DOWN_MASK ) );
        decreaseSize.addActionListener( e -> {
            int activeTabIdx = tabsPanel.getSelectedIndex();
            JTextArea currTextArea = tabsList.get( activeTabIdx ).getTextArea();
            Font oldFont = currTextArea.getFont();
            Font newFont = new Font( oldFont.getName(), oldFont.getStyle(), oldFont.getSize() - 1 );
            currTextArea.setFont( newFont );
        });
        JMenuItem wrapLines = new JMenuItem( "Wrap lines" );
        wrapLines.addActionListener( e -> {
            JTextArea currTextArea = tabsList.get( tabsPanel.getSelectedIndex() ).getTextArea();
            currTextArea.setLineWrap( !currTextArea.getLineWrap() );
        });

        format.add( wrapLines );
        format.add( increaseSize );
        format.add( decreaseSize );

        return format;
    }

    private JMenu setupHelpSubmenu(){

        JMenu help = new JMenu("Help");
        JMenuItem helpItem = new JMenuItem( "Show help tab");
        helpItem.addActionListener( e -> {
            if( helpTabIdx == -1 ){
                addTab( new HelpTab() );
            }
            tabsPanel.setSelectedIndex( helpTabIdx );
        });
        help.add( helpItem );
        return help;
    }

    private void setupMenu() {

        menu = new JMenuBar();
        JMenu file = setupFileSubmenu();
        JMenu edit = setupEditSubmenu();
        JMenu format = setupFormatSubmenu();
        JMenu help = setupHelpSubmenu();
        menu.add(file);
        menu.add(edit);
        menu.add(format);
        menu.add(help);
    }

    public MyEditor( String windowTitle ) {
        frame = new JFrame(windowTitle);
        frame.setDefaultCloseOperation(WindowConstants.EXIT_ON_CLOSE);
        tabsList = new LinkedList<>();

        setupMenu();
        frame.setJMenuBar(menu);
        addTab( new HelpTab() );

        frame.add(mainPanel);
        frame.pack();
        frame.setVisible(true);
        frame.setExtendedState( frame.getExtendedState() | JFrame.MAXIMIZED_BOTH );
    }

    public static void main( String[] args ) {
        MyEditor editor = new MyEditor( "Prosty Edytor" );
    }
}
