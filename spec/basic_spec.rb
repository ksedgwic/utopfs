require 'spec'


describe "A utopfs filesytem" do

  before(:each) do
    @mdir = "./tmp/mount"
 #   @mdir = "./tmp/mount#{rand(999999999)}"    
  end
  
  after(:each) do
    unmount if is_mounted?
  end
   
  def mount(mdir = @mdir)    
    FileUtils.mkdir_p mdir
    `../utopfs/Linux.DBGOBJ/utopfs #{mdir}` if ! is_mounted?(mdir)
  end
  
  def unmount(mdir = @mdir)
    `fusermount -u #{mdir}`
  end
  
  def is_mounted?(mdir = @mdir)       
    File.exists?(mdir) && Dir.entries(mdir).include?(".utopfs")
  end

  it "can be mounted" do
    is_mounted?.should == false    
    mount
    is_mounted?.should == true
  end

  it "can be mounted and unmounted" do   
    mount    
    unmount.should == ""
  end
  
  describe "which is mounted" do
    before(:each) do
      mount
    end
    it "should have a .utopfs directory" do
      Dir.entries(@mdir).include?(".utopfs").should == true    
    end
    it "should have a .utopfs/version file that contains a version string" do
      File.exists?("#{@mdir}/.utopfs/version").should == true    
      File.readlines("#{@mdir}/.utopfs/version")[0]["version"].should_not == nil
    end
  end
  
end

