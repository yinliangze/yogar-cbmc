class assert1
{
  public static void main(String[] args)
  {
    java.util.Random random = new java.util.Random(42);
    
    int i=random.nextInt();
    
    if(i>=10)
      assert i>=20 : "my super assertion"; // should hold
  }
}

